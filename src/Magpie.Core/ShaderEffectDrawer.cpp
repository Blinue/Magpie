#include "pch.h"
#include "CommandContext.h"
#include "D3D12Context.h"
#include "DescriptorHeap.h"
#include "DirectXHelper.h"
#include "EffectsService.h"
#include "Logger.h"
#include "TextureHelper.h"
#include "ScalingOptions.h"
#include "ScalingWindow.h"
#include "ShaderEffectDrawer.h"
#include "ShaderEffectDrawInfo.h"
#include "StrHelper.h"
#include "AppFolderManager.h"
// Conan 的 muparser 不含 UNICODE 支持
#pragma push_macro("_UNICODE")
#undef _UNICODE
#include <muParser.h>
#pragma pop_macro("_UNICODE")

namespace Magpie {

ShaderEffectDrawer::~ShaderEffectDrawer() noexcept {
#ifdef _DEBUG
	if (_descriptorBaseOffset != std::numeric_limits<uint32_t>::max()) {
		_d3d12Context->GetDescriptorHeap().Free(
			_descriptorBaseOffset, (uint32_t)_textureDescriptorMap.size());
	}
#endif

	if (!_compilationTaskId.empty()) {
		EffectsService::Get().ReleaseTask(_compilationTaskId);
	}
}

const EffectInfo* ShaderEffectDrawer::Initialize(
	D3D12Context& d3d12Context,
	const EffectOption& effectOption
) noexcept {
	_d3d12Context = &d3d12Context;
	_effectOption = &effectOption;

	_effectInfo = EffectsService::Get().GetEffect(effectOption.name);
	if (!_effectInfo) {
		Logger::Get().Error("EffectsService::GetEffect 失败");
		return nullptr;
	}

	return _effectInfo;
}

void ShaderEffectDrawer::Bind(SizeU inputSize, SizeU outputSize, const ColorInfo& colorInfo) noexcept {
	// 确保输出尺寸合法
	assert(!(_effectInfo->scaleFactor != 0 &&
		(inputSize.width * _effectInfo->scaleFactor != outputSize.width ||
			inputSize.height * _effectInfo->scaleFactor != outputSize.height)));

	if (!_errorMsg.empty()) {
		return;
	}

	// 如果色域改变可能需要重新编译，否则只需要重新创建中间纹理
	if (_inputSize == inputSize && _outputSize == outputSize) {
		if (_colorInfo == colorInfo) {
			return;
		}
	} else {
		_inputSize = inputSize;
		_outputSize = outputSize;
		_shouldUpdateSizeDependentResources = true;
	}

	bool wasSrgb = _colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;
	bool isSrgb = colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;
	_colorInfo = colorInfo;

	if (!_compilationTaskId.empty()) {
		if (wasSrgb == isSrgb) {
			return;
		}

		EffectsService::Get().ReleaseTask(_compilationTaskId);
		_drawInfo = nullptr;
	}

	const ScalingOptions& options = ScalingWindow::Get().Options();
	_compilationTaskId = EffectsService::Get().SubmitCompileShaderEffectTask(
		_effectOption->name,
		options.IsInlineParams() ? &_effectOption->parameters : nullptr,
		_d3d12Context->GetShaderModel(),
		_d3d12Context->IsMinFloat16Supported(),
		_d3d12Context->IsNative16BitSupported(),
		colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange,
		options.IsSaveEffectSources(),
		options.IsWarningsAreErrors(),
		options.IsEffectCacheDisabled()
	);
	if (_compilationTaskId.empty()) {
		_errorMsg = "编译失败";
		Logger::Get().Error("EffectsService::SubmitCompileShaderEffectTask 失败");
	}
}

HRESULT ShaderEffectDrawer::Update(EffectDrawerState& state, std::string& message) noexcept {
	if (!_errorMsg.empty()) {
		state = EffectDrawerState::Error;
		message = _errorMsg;
		return S_OK;
	}

	if (_drawInfo) {
		if (_shouldUpdateSizeDependentResources) {
			HRESULT hr = _UpdateSizeDependentResources();
			if (FAILED(hr)) {
				Logger::Get().ComError("_UpdateSizeDependentResources 失败", hr);
				return hr;
			}

			if (!_errorMsg.empty()) {
				state = EffectDrawerState::Error;
				message = _errorMsg;
				return S_OK;
			}
		}

		state = EffectDrawerState::Ready;
		return S_OK;
	}

	if (!EffectsService::Get().GetTaskResult(_compilationTaskId, &_drawInfo)) {
		state = EffectDrawerState::Error;
		return S_OK;
	}

	if (!_drawInfo) {
		state = EffectDrawerState::NotReady;
		return S_OK;
	}

	HRESULT hr = _CreateDeviceResources();
	if (FAILED(hr)) {
		Logger::Get().ComError("_CreateDeviceResources 失败", hr);
		return hr;
	}

	if (!_errorMsg.empty()) {
		_drawInfo = nullptr;
		state = EffectDrawerState::Error;
		return S_OK;
	}

	state = EffectDrawerState::Ready;
	return S_OK;
}

HRESULT ShaderEffectDrawer::Draw(
	ComputeContext& computeContext,
	uint32_t inputSrvOffset,
	uint32_t outputUavOffset
) noexcept {
	assert(_drawInfo);

	if (_shouldUpdateConstantBuffer) {
		_shouldUpdateConstantBuffer = false;

		computeContext.CopyBufferRegion(_constantBuffer.get(), 0,
			_constantUploadBuffer.get(), 0, _constantsDataSize, false);

		computeContext.InsertTransitionBarrier(
			_constantBuffer.get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
		);
	}

	if (!_textureSourceDatas.empty()) {
		for (const _TextureSourceData& sourceData : _textureSourceDatas) {
			if (_textureStates[sourceData.targetTextureIdx] != D3D12_RESOURCE_STATE_COPY_DEST) {
				continue;
			}

			const ShaderEffectTextureDesc& effectTexDesc = _drawInfo->textures[sourceData.targetTextureIdx];
			DXGI_FORMAT format = SHADER_TEXTURE_FORMAT_PROPS[(uint32_t)effectTexDesc.format].dxgiFormat;

			CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
				format, sourceData.textureSize.width, sourceData.textureSize.height, 1, 1);

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureLayout;
			_d3d12Context->GetDevice()->GetCopyableFootprints(&texDesc, 0, 1, 0,
				&textureLayout, nullptr, nullptr, nullptr);

			computeContext.CopyTextureRegion(
				CD3DX12_TEXTURE_COPY_LOCATION(_textures[sourceData.targetTextureIdx].get()),
				0,
				0,
				CD3DX12_TEXTURE_COPY_LOCATION(sourceData.uploadBuffer.get(), textureLayout)
			);

			computeContext.InsertTransitionBarrier(
				_textures[sourceData.targetTextureIdx].get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			);
			_textureStates[sourceData.targetTextureIdx] = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}
	}

	for (uint32_t i = 0; i < _passDatas.size(); ++i) {
		const _PassData& curPassData = _passDatas[i];
		const ShaderEffectPassDesc& passDesc = _drawInfo->passes[i];

		computeContext.SetPipelineState(curPassData.pso.get());
		computeContext.SetRootSignature(curPassData.rootSignature.get());

		// 合并状态转换
		for (uint32_t input : passDesc.inputs) {
			if (input == 0) {
				continue;
			}

			auto oldState = std::exchange(_textureStates[size_t(input - 2)],
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			if (oldState != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {
				computeContext.InsertTransitionBarrier(
					_textures[size_t(input - 2)].get(),
					oldState,
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
				);
			}
		}

		for (uint32_t output : passDesc.outputs) {
			if (output == 1) {
				continue;
			}

			auto oldState = std::exchange(_textureStates[size_t(output - 2)],
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			if (oldState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
				computeContext.InsertTransitionBarrier(
					_textures[size_t(output - 2)].get(),
					oldState,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS
				);
			}
		}

		computeContext.SetComputeRootConstantBufferView(0, _constantBuffer->GetGPUVirtualAddress());

		uint32_t rootParameterIndex = 1;

		if (!passDesc.inputs.empty() && passDesc.inputs[0] == 0) {
			computeContext.SetRootDescriptorTable(rootParameterIndex++, inputSrvOffset);
		}

		if (passDesc.outputs[0] == 1) {
			computeContext.SetRootDescriptorTable(rootParameterIndex++, outputUavOffset);
		}

		// 不需要额外的描述符时 passData.descriptorBaseOffset 为 UINT_MAX
		if (curPassData.descriptorBaseOffset != std::numeric_limits<uint32_t>::max()) {
			computeContext.SetRootDescriptorTable(rootParameterIndex++, curPassData.descriptorBaseOffset);
		}

		computeContext.Dispatch(curPassData.dispatchCount.first, curPassData.dispatchCount.second);
	}

	return S_OK;
}

HRESULT ShaderEffectDrawer::_CreateDeviceResources() noexcept {
	ID3D12Device5* device = _d3d12Context->GetDevice();
	const ScalingOptions& options = ScalingWindow::Get().Options();

	const uint32_t passCount = (uint32_t)_drawInfo->passes.size();
	_passDatas.resize(passCount);

	for (uint32_t passIdx = 0; passIdx < passCount; ++passIdx) {
		_PassData& curPassData = _passDatas[passIdx];
		const ShaderEffectPassDesc& curPassDesc = _drawInfo->passes[passIdx];
		
		winrt::com_ptr<ID3DBlob> signature;

		std::array<D3D12_ROOT_PARAMETER1, 4> rootParams{};
		uint32_t curRootParamIdx = 0;
		std::array<D3D12_DESCRIPTOR_RANGE1, 4> descriptorRanges{};
		uint32_t curDescriptorRangeIdx = 0;

		rootParams[curRootParamIdx++] = D3D12_ROOT_PARAMETER1{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
			.Descriptor = {
				.ShaderRegister = 0,
				.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
			}
		};

		// INPUT 和 OUTPUT 的描述符独立， 其他描述符连续
		std::span<const uint32_t> otherInputs;
		std::span<const uint32_t> otherOutputs;
		if (!curPassDesc.inputs.empty()) {
			if (curPassDesc.inputs[0] == 0) {
				descriptorRanges[curDescriptorRangeIdx] = CD3DX12_DESCRIPTOR_RANGE1(
					D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
					D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);

				rootParams[curRootParamIdx++] = D3D12_ROOT_PARAMETER1{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					.DescriptorTable = {
						.NumDescriptorRanges = 1,
						.pDescriptorRanges = &descriptorRanges[curDescriptorRangeIdx]
					}
				};

				++curDescriptorRangeIdx;
				otherInputs = std::span(curPassDesc.inputs.begin() + 1, curPassDesc.inputs.end());
			} else {
				otherInputs = curPassDesc.inputs;
			}
		}

		assert(!curPassDesc.outputs.empty());
		if (curPassDesc.outputs[0] == 1) {
			descriptorRanges[curDescriptorRangeIdx] = CD3DX12_DESCRIPTOR_RANGE1(
				D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

			rootParams[curRootParamIdx++] = D3D12_ROOT_PARAMETER1{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				.DescriptorTable = {
					.NumDescriptorRanges = 1,
					.pDescriptorRanges = &descriptorRanges[curDescriptorRangeIdx]
				}
			};

			++curDescriptorRangeIdx;
			otherOutputs = std::span(curPassDesc.outputs.begin() + 1, curPassDesc.outputs.end());
		} else {
			otherOutputs = curPassDesc.outputs;
		}
		
		if (!otherInputs.empty() || !otherOutputs.empty()) {
			const uint32_t startIdx = curDescriptorRangeIdx;

			if (!otherInputs.empty()) {
				descriptorRanges[curDescriptorRangeIdx++] = CD3DX12_DESCRIPTOR_RANGE1(
					D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
					(UINT)otherInputs.size(),
					UINT(otherInputs.data() != curPassDesc.inputs.data()),
					0,
					D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
				);
			}

			if (!otherOutputs.empty()) {
				descriptorRanges[curDescriptorRangeIdx++] = CD3DX12_DESCRIPTOR_RANGE1(
					D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
					(UINT)otherOutputs.size(),
					UINT(otherOutputs.data() != curPassDesc.outputs.data()),
					0,
					D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE
				);
			}

			rootParams[curRootParamIdx++] = D3D12_ROOT_PARAMETER1{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				.DescriptorTable = {
					.NumDescriptorRanges = curDescriptorRangeIdx - startIdx,
					.pDescriptorRanges = &descriptorRanges[startIdx]
				}
			};
		}

		SmallVector<D3D12_STATIC_SAMPLER_DESC> samplerDescs(_drawInfo->samplers.size());
		for (size_t i = 0; i < samplerDescs.size(); ++i) {
			const ShaderEffectSamplerDesc& samplerDesc = _drawInfo->samplers[i];

			D3D12_TEXTURE_ADDRESS_MODE addressMode =
				samplerDesc.addressType == ShaderEffectSamplerAddressType::Clamp ?
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP : D3D12_TEXTURE_ADDRESS_MODE_WRAP;

			samplerDescs[i] = D3D12_STATIC_SAMPLER_DESC{
				.Filter = samplerDesc.filterType == ShaderEffectSamplerFilterType::Point ?
					D3D12_FILTER_MIN_MAG_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				.AddressU = addressMode,
				.AddressV = addressMode,
				.AddressW = addressMode,
				.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
				.ShaderRegister = (UINT)i
			};
		}

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
			(UINT)curRootParamIdx, rootParams.data(),
			(UINT)samplerDescs.size(), samplerDescs.data());

		HRESULT hr = D3DX12SerializeVersionedRootSignature(
			&rootSignatureDesc,
			_d3d12Context->GetRootSignatureVersion(),
			signature.put(),
			nullptr
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("D3DX12SerializeVersionedRootSignature 失败", hr);
			return hr;
		}

		hr = device->CreateRootSignature(
			0,
			signature->GetBufferPointer(),
			signature->GetBufferSize(),
			IID_PPV_ARGS(&curPassData.rootSignature)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateRootSignature 失败", hr);
			return hr;
		}

		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {
			.pRootSignature = curPassData.rootSignature.get(),
			.CS = CD3DX12_SHADER_BYTECODE(curPassDesc.byteCode.get())
		};
		hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&curPassData.pso));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateComputePipelineState 失败", hr);
			return hr;
		}
	}

	// 描述符布局不会变化
	if (_descriptorBaseOffset == std::numeric_limits<uint32_t>::max()) {
		// [(通道序号, 需要的描述符数量)]
		SmallVector<std::pair<uint32_t, uint32_t>> descriptorCounts;

		for (uint32_t passIdx = 0; passIdx < passCount; ++passIdx) {
			const ShaderEffectPassDesc& curPassDesc = _drawInfo->passes[passIdx];

			uint32_t curPassDescriptorCount =
				(uint32_t)curPassDesc.inputs.size() + (uint32_t)curPassDesc.outputs.size();

			if (!curPassDesc.inputs.empty() && curPassDesc.inputs[0] == 0) {
				--curPassDescriptorCount;
			}
			if (curPassDesc.outputs[0] == 1) {
				--curPassDescriptorCount;
			}

			if (curPassDescriptorCount != 0) {
				descriptorCounts.emplace_back(passIdx, curPassDescriptorCount);
			}
		}

		// 不需要额外的描述符时 passData.descriptorBaseOffset 为 UINT_MAX
		if (!descriptorCounts.empty()) {
			// 按需要的描述符数量从多到少排序，这可以提高复用描述符的概率
			std::sort(
				descriptorCounts.begin(),
				descriptorCounts.end(),
				[](const auto& pair1, const auto& pair2) {
					return pair1.second > pair2.second;
				}
			);

			for (const auto& pair : descriptorCounts) {
				const ShaderEffectPassDesc& curPassDesc = _drawInfo->passes[pair.first];
				SmallVector<uint32_t> curPassDescriptors;

				if (!curPassDesc.inputs.empty()) {
					auto begin = curPassDesc.inputs.begin() + uint32_t(curPassDesc.inputs[0] == 0);
					auto end = curPassDesc.inputs.end();
					curPassDescriptors.reserve(curPassDescriptors.size() + (end - begin));
					for (auto it = begin; it != end; ++it) {
						curPassDescriptors.push_back(*it - 2);
					}
				}

				{
					auto begin = curPassDesc.outputs.begin() + uint32_t(curPassDesc.outputs[0] == 1);
					auto end = curPassDesc.outputs.end();
					curPassDescriptors.reserve(curPassDescriptors.size() + (end - begin));
					uint32_t textureCount = (uint32_t)_drawInfo->textures.size();
					for (auto it = begin; it != end; ++it) {
						// 为了和 SRV 做区分，UAV 的索引加上纹理总数
						curPassDescriptors.push_back(*it - 2 + textureCount);
					}
				}

				assert(curPassDescriptors.size() == pair.second);

				// 寻找可以复用的区域
				auto it = std::search(
					_textureDescriptorMap.begin(),
					_textureDescriptorMap.end(),
					curPassDescriptors.begin(),
					curPassDescriptors.end()
				);
				if (it == _textureDescriptorMap.end()) {
					_passDatas[pair.first].descriptorBaseOffset = (uint32_t)_textureDescriptorMap.size();

					_textureDescriptorMap.insert(
						_textureDescriptorMap.end(),
						curPassDescriptors.begin(),
						curPassDescriptors.end()
					);
				} else {
					_passDatas[pair.first].descriptorBaseOffset =
						uint32_t(it - _textureDescriptorMap.begin());
				}
			}
		}

		if (!_textureDescriptorMap.empty()) {
			HRESULT hr = _d3d12Context->GetDescriptorHeap()
				.Alloc((uint32_t)_textureDescriptorMap.size(), _descriptorBaseOffset);
			if (FAILED(hr)) {
				Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
				return hr;
			}

			for (_PassData& passData : _passDatas) {
				if (passData.descriptorBaseOffset != std::numeric_limits<uint32_t>::max()) {
					passData.descriptorBaseOffset += _descriptorBaseOffset;
				}
			}
		}
	}

	// 常量缓冲区可以复用
	if (!_constantBuffer) {
		// 10 个内置常量
		uint32_t paramCount = 10;
		// PS 样式需要额外常量，除了最后一个通道
		for (uint32_t i = 0, end = (uint32_t)_drawInfo->passes.size() - 1; i < end; ++i) {
			if (bool(_drawInfo->passes[i].flags & ShaderEffectPassFlags::PSStyle)) {
				paramCount += 2;
			}
		}
		// WCG/HDR 需要额外常量，始终预留位置使常量缓冲区可以复用
		paramCount += 2;
		// 未启用内联参数时每个参数占用一个常量
		if (!options.IsInlineParams()) {
			paramCount += (uint32_t)_effectInfo->params.size();
		}
		
		D3D12_HEAP_FLAGS heapFlag = _d3d12Context->IsHeapFlagCreateNotZeroedSupported() ?
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

		CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);

		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(UINT64(paramCount * 4));

		HRESULT hr = device->CreateCommittedResource(
			&heapProperties,
			heapFlag,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&_constantUploadBuffer)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommittedResource 失败", hr);
			return hr;
		}

		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		// 常量缓冲区必须对齐到 256 字节
		bufferDesc.Width = (bufferDesc.Width + 255) & ~255;
		hr = device->CreateCommittedResource(
			&heapProperties,
			heapFlag,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&_constantBuffer)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommittedResource 失败", hr);
			return hr;
		}

		// 无需解除映射
		D3D12_RANGE readRange{};
		hr = _constantUploadBuffer->Map(0, &readRange, &_constantUploadBufferData);
		if (FAILED(hr)) {
			Logger::Get().ComError("ID3D12Resource::Map 失败", hr);
			return hr;
		}
	}

	HRESULT hr = _UpdateSizeDependentResources();
	if (FAILED(hr)) {
		Logger::Get().ComError("_UpdateSizeDependentResources 失败", hr);
		return hr;
	}

	return S_OK;
}

// 常量缓冲区布局如下：
// uint2 __inputSize;
// uint2 __outputSize;
// float2 __inputPt;
// float2 __outputPt;
// float2 __scale;
// ↓ PS 样式参数 ↓
// [float2 __pass1OutputPt;]
// [float2 __pass2OutputPt;]
// [...]
// ↓ WCG/HDR 参数 ↓
// [float __maxLuminance;]
// [float __sdrWhiteLevel;]
// ↓ 效果参数 ↓
// [float param1;]
// [uint param2;]
// [...]
// 此方法只更新 _constantUploadBuffer，_constantBuffer 将在 Draw 中更新。
void ShaderEffectDrawer::_UpdateConstants() noexcept {
	assert(_constantBuffer);

	using Constant32 = DirectXHelper::Constant32;
	// SmallVector 中数据地址偏移量是 16 字节，alignas(16) 使 SmallVector
	// 和其中数据都对齐到 16 字节边界。
	alignas(16) SmallVector<Constant32, 32> constants;
	assert((uint8_t*)constants.data() - (uint8_t*)&constants == 16);

	constants.emplace_back(Constant32::UInt(_inputSize.width));
	constants.emplace_back(Constant32::UInt(_inputSize.height));
	constants.emplace_back(Constant32::UInt(_outputSize.width));
	constants.emplace_back(Constant32::UInt(_outputSize.height));
	constants.emplace_back(Constant32::Float(1.0f / _inputSize.width));
	constants.emplace_back(Constant32::Float(1.0f / _inputSize.height));
	constants.emplace_back(Constant32::Float(1.0f / _outputSize.width));
	constants.emplace_back(Constant32::Float(1.0f / _outputSize.height));
	constants.emplace_back(Constant32::Float((float)_outputSize.width / _inputSize.width));
	constants.emplace_back(Constant32::Float((float)_outputSize.height / _inputSize.height));

	// PS 样式参数
	for (uint32_t i = 0, end = (uint32_t)_drawInfo->passes.size() - 1; i < end; ++i) {
		if (bool(_drawInfo->passes[i].flags & ShaderEffectPassFlags::PSStyle)) {
			uint32_t output = _drawInfo->passes[i].outputs[0];
			if (output == 1) {
				constants.emplace_back(Constant32::Float(1.0f / _outputSize.width));
				constants.emplace_back(Constant32::Float(1.0f / _outputSize.height));
			} else {
				D3D12_RESOURCE_DESC texDesc = _textures[size_t(output - 2)]->GetDesc();
				constants.emplace_back(Constant32::Float(1.0f / texDesc.Width));
				constants.emplace_back(Constant32::Float(1.0f / texDesc.Height));
			}
		}
	}

	// WCG/HDR 参数
	if (bool(_effectInfo->flags & EffectFlags::SupportAdvancedColor) &&
		_colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange) {
		constants.emplace_back(Constant32::Float(_colorInfo.maxLuminance));
		constants.emplace_back(Constant32::Float(_colorInfo.sdrWhiteLevel));
	}

	// 效果参数
	if (!ScalingWindow::Get().Options().IsInlineParams()) {
		for (const EffectParameterDesc& paramDesc : _effectInfo->params) {
			auto it = _effectOption->parameters.find(paramDesc.name);
			float value = it == _effectOption->parameters.end() ? paramDesc.defaultValue : it->second;
			switch (paramDesc.type) {
			case EffectParameterType::Float:
				constants.emplace_back(Constant32::Float(value));
				break;
			case EffectParameterType::Int:
				constants.emplace_back(Constant32::Int(std::lround(value)));
				break;
			default:
				assert(paramDesc.type == EffectParameterType::UInt);
				constants.emplace_back(Constant32::UInt((uint32_t)std::lround(value)));
				break;
			}
		}
	}

	_constantsDataSize = (uint32_t)constants.size() * 4;
	assert(_constantsDataSize <= _constantUploadBuffer->GetDesc().Width);
	std::memcpy(_constantUploadBufferData, constants.data(), _constantsDataSize);

	_shouldUpdateConstantBuffer = true;
}

HRESULT ShaderEffectDrawer::_UpdateSizeDependentResources() noexcept {
	_shouldUpdateSizeDependentResources = false;

	const uint32_t textureCount = (uint32_t)_drawInfo->textures.size();
	if (textureCount != 0) {
		ID3D12Device5* device = _d3d12Context->GetDevice();

		_textures.resize(textureCount);
		_textureStates.resize(textureCount);

		mu::Parser exprParser;
		exprParser.DefineConst("INPUT_WIDTH", _inputSize.width);
		exprParser.DefineConst("INPUT_HEIGHT", _inputSize.height);
		exprParser.DefineConst("OUTPUT_WIDTH", _outputSize.width);
		exprParser.DefineConst("OUTPUT_HEIGHT", _outputSize.height);

		for (uint32_t texIdx = 0; texIdx < textureCount; ++texIdx) {
			const ShaderEffectTextureDesc& effectTexDesc = _drawInfo->textures[texIdx];

			DXGI_FORMAT dxgiFormat;

			if (effectTexDesc.source.empty()) {
				SizeU texSize{};
				try {
					exprParser.SetExpr(effectTexDesc.widthExpr);
					long width = std::lround(exprParser.Eval());
					exprParser.SetExpr(effectTexDesc.heightExpr);
					long height = std::lround(exprParser.Eval());

					if (width > 0 && height > 0) {
						texSize.width = (uint32_t)width;
						texSize.height = (uint32_t)height;
					}
				} catch (const mu::ParserError& e) {
					Logger::Get().Error(fmt::format("计算纹理 {} 尺寸失败: {}",
						effectTexDesc.name, e.GetMsg()));
				}

				if (texSize.width == 0) {
					_errorMsg = fmt::format("计算纹理 {} 尺寸失败", effectTexDesc.name);
					return S_OK;
				}

				if (effectTexDesc.format == ShaderEffectTextureFormat::COLOR_SPACE_ADAPTIVE) {
					if (_colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange) {
						dxgiFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
					} else {
						dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
					}
				} else {
					dxgiFormat = SHADER_TEXTURE_FORMAT_PROPS[(uint32_t)effectTexDesc.format].dxgiFormat;
				}

				if (_textures[texIdx]) {
					D3D12_RESOURCE_DESC texDesc = _textures[texIdx]->GetDesc();
					if (texDesc.Width == texSize.width && texDesc.Height == texSize.height &&
						texDesc.Format == dxgiFormat) {
						continue;
					}
				}

				CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

				D3D12_HEAP_FLAGS heapFlags = _d3d12Context->IsHeapFlagCreateNotZeroedSupported() ?
					D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

				CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(dxgiFormat,
					texSize.width, texSize.height, 1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
				);

				HRESULT hr = device->CreateCommittedResource(&heapProps, heapFlags, &texDesc,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&_textures[texIdx]));
				if (FAILED(hr)) {
					Logger::Get().ComError("CreateCommittedResource 失败", hr);
					return hr;
				}

				_textureStates[texIdx] = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			} else {
				if (_textures[texIdx]) {
					continue;
				}

				dxgiFormat = SHADER_TEXTURE_FORMAT_PROPS[(uint32_t)effectTexDesc.format].dxgiFormat;

				size_t delimPos = _effectInfo->name.find_last_of('\\');
				std::wstring texPath = StrHelper::UTF8ToUTF16(delimPos == std::string::npos ?
					effectTexDesc.source : StrHelper::Concat(
						std::string_view(_effectInfo->name.c_str(), delimPos + 1), effectTexDesc.source));

				_TextureSourceData& sourceData = _textureSourceDatas.emplace_back();
				// 可能会把 dxgiFormat 修改为 sRGB
				sourceData.uploadBuffer = TextureHelper::LoadFromFile(
					AppFolderManager::Get().GetBuiltInShaderEffectsDir() / texPath,
					*_d3d12Context,
					dxgiFormat,
					sourceData.textureSize
				);
				if (!sourceData.uploadBuffer) {
					Logger::Get().Error("TextureHelper::LoadFromFile 失败");
					_errorMsg = fmt::format("加载纹理 {} 数据失败", effectTexDesc.name);
					return S_OK;
				}

				CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

				D3D12_HEAP_FLAGS heapFlags = _d3d12Context->IsHeapFlagCreateNotZeroedSupported() ?
					D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

				CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
					dxgiFormat, sourceData.textureSize.width, sourceData.textureSize.height,
					1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE);

				HRESULT hr = device->CreateCommittedResource(&heapProps, heapFlags, &texDesc,
					D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&_textures[texIdx]));
				if (FAILED(hr)) {
					Logger::Get().ComError("CreateCommittedResource 失败", hr);
					return hr;
				}

				sourceData.targetTextureIdx = texIdx;
				_textureStates[texIdx] = D3D12_RESOURCE_STATE_COPY_DEST;
			}

			if (!_textureDescriptorMap.empty()) {
				auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();
				const uint32_t descriptorSize = descriptorHeap.GetDescriptorSize();

				CD3DX12_CPU_DESCRIPTOR_HANDLE baseHandle(
					descriptorHeap.GetCpuHandle(_descriptorBaseOffset));

				CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc =
					CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(dxgiFormat, 1);

				for (auto it = _textureDescriptorMap.begin();; ++it) {
					it = std::find(it, _textureDescriptorMap.end(), texIdx);
					if (it == _textureDescriptorMap.end()) {
						break;
					}

					size_t offset = it - _textureDescriptorMap.begin();
					device->CreateShaderResourceView(_textures[texIdx].get(), &srvDesc,
						CD3DX12_CPU_DESCRIPTOR_HANDLE(baseHandle, (INT)offset, descriptorSize));
				}

				// 从文件加载的纹理不能作为输出
				if (effectTexDesc.source.empty()) {
					// 为了和 SRV 做区分，UAV 的索引加上纹理总数
					uint32_t uavIdx = texIdx + textureCount;
					CD3DX12_UNORDERED_ACCESS_VIEW_DESC uavDesc =
						CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2D(dxgiFormat);

					for (auto it = _textureDescriptorMap.begin();; ++it) {
						it = std::find(it, _textureDescriptorMap.end(), uavIdx);
						if (it == _textureDescriptorMap.end()) {
							break;
						}

						size_t offset = it - _textureDescriptorMap.begin();
						device->CreateUnorderedAccessView(_textures[texIdx].get(), nullptr, &uavDesc,
							CD3DX12_CPU_DESCRIPTOR_HANDLE(baseHandle, (INT)offset, descriptorSize));
					}
				} else {
					assert(std::find(
						_textureDescriptorMap.begin(),
						_textureDescriptorMap.end(),
						texIdx + textureCount
					) == _textureDescriptorMap.end());
				}
			}
		}
	}
	
	for (uint32_t passIdx = 0; passIdx < _passDatas.size(); ++passIdx) {
		const ShaderEffectPassDesc& curPassDesc = _drawInfo->passes[passIdx];

		SizeU outputSize;
		if (curPassDesc.outputs[0] == 1) {
			outputSize = _outputSize;
		} else {
			D3D12_RESOURCE_DESC texDesc = _textures[size_t(curPassDesc.outputs[0] - 2)]->GetDesc();
			outputSize = { (uint32_t)texDesc.Width,(uint32_t)texDesc.Height };
		}

		_passDatas[passIdx].dispatchCount = {
			(outputSize.width + curPassDesc.blockSize.width - 1) / curPassDesc.blockSize.width,
			(outputSize.height + curPassDesc.blockSize.height - 1) / curPassDesc.blockSize.height
		};
	}

	_UpdateConstants();
	return S_OK;
}

}
