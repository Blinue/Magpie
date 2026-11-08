#include "pch.h"
#include "CommandContext.h"
#include "D3D12Context.h"
#include "DescriptorHeap.h"
#include "DirectXHelper.h"
#include "EffectsMemPoolBuilder.h"
#include "EffectsService.h"
#include "GpuResourceManager.h"
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
	if (_descriptorBaseOffset != UINT_MAX) {
		_d3d12Context->GetDescriptorHeap().Free(_descriptorBaseOffset, _descriptorCount);
	}
#endif

	if (!_compilationTaskId.empty()) {
		EffectsService::Get().ReleaseTask(_compilationTaskId);
	}
}

HRESULT ShaderEffectDrawer::Initialize(
	D3D12Context& d3d12Context,
	const EffectOption& effectOption,
	EffectDrawerInfo& effectDrawerInfo
) noexcept {
	_d3d12Context = &d3d12Context;
	_effectOption = &effectOption;

	_effectInfo = EffectsService::Get().GetEffect(effectOption.name);
	if (!_effectInfo) {
		Logger::Get().Error("EffectsService::GetEffect 失败");
		return E_FAIL;
	}

	effectDrawerInfo.scaleFactor = _effectInfo->scaleFactor;
	effectDrawerInfo.minFrameRate = _effectInfo->minFrameRate;
	effectDrawerInfo.flags = EffectDrawerFlags::SupportOutputAliasing;
	return S_OK;
}

void ShaderEffectDrawer::Bind(SizeU inputSize, SizeU outputSize, const ColorInfo& colorInfo) noexcept {
	// 确保输出尺寸合法
	assert(!(_effectInfo->scaleFactor != 0 &&
		(inputSize.width * _effectInfo->scaleFactor != outputSize.width ||
			inputSize.height * _effectInfo->scaleFactor != outputSize.height)));

	// 如果上一次计算的纹理尺寸不合法则再次尝试
	if (_error != _Error::NoError && _error != _Error::InvalidTextureSize) {
		return;
	}

	// 如果色域改变可能需要重新编译，否则只需要重新创建中间纹理
	if (_inputSize == inputSize && _outputSize == outputSize) {
		if (_acKind == colorInfo.kind &&
			_maxLuminance == colorInfo.maxLuminance &&
			_sdrWhiteLevel == colorInfo.sdrWhiteLevel)
		{
			return;
		}
	} else {
		_inputSize = inputSize;
		_outputSize = outputSize;
		_shouldUpdateTextures = true;
	}

	bool wasSrgb = _acKind != winrt::AdvancedColorKind::StandardDynamicRange;
	bool isSrgb = colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;

	_acKind = colorInfo.kind;
	_maxLuminance = colorInfo.maxLuminance;
	_sdrWhiteLevel = colorInfo.sdrWhiteLevel;

	_error = _Error::NoError;

	if (!_compilationTaskId.empty()) {
		if (wasSrgb == isSrgb) {
			return;
		}

		EffectsService::Get().ReleaseTask(_compilationTaskId);
		_drawInfo = nullptr;
	}

	_shouldUpdateTextures = true;

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
		_error = _Error::CompilationFailed;
		Logger::Get().Error("EffectsService::SubmitCompileShaderEffectTask 失败");
	}
}

HRESULT ShaderEffectDrawer::Update(EffectDrawerState& state, EffectDebugInfo& debugInfo) noexcept {
	if (_error != _Error::NoError) {
		state = EffectDrawerState::Error;
		return S_OK;
	}

	bool shouldBuildMemPool = false;

	if (!_drawInfo) {
		if (!EffectsService::Get().GetTaskResult(_compilationTaskId, &_drawInfo)) {
			_error = _Error::CompilationFailed;
			state = EffectDrawerState::Error;
			return S_OK;
		}

		if (!_drawInfo) {
			state = EffectDrawerState::NotReady;
			return S_OK;
		}

		// 初始化不会改变的资源
		HRESULT hr = _InitializeCommonResources();
		if (FAILED(hr)) {
			Logger::Get().ComError("_InitializeCommonResources 失败", hr);
			return hr;
		}

		if (_error != _Error::NoError) {
			state = EffectDrawerState::Error;
			return S_OK;
		}
		
		hr = _CreatePSOs();
		if (FAILED(hr)) {
			Logger::Get().ComError("_CreatePSOs 失败", hr);
			return hr;
		}

		if (_hasNonPersistentResource) {
			// 如果有纹理的格式变了则需要重新构建内存池
			for (const ShaderEffectTextureDesc& effectTexDesc : _drawInfo->textures) {
				if (effectTexDesc.format == EffectTextureFormat::COLOR_SPACE_ADAPTIVE) {
					shouldBuildMemPool = true;
					break;
				}
			}
		}
	}

	if (_shouldUpdateTextures) {
		_shouldUpdateTextures = false;

		if (_hasNonPersistentResource) {
			if (_ComputeNonPersistentTextureSizes()) {
				shouldBuildMemPool = true;
			}
			
			if (_error != _Error::NoError) {
				state = EffectDrawerState::Error;
				return S_OK;
			}
		}
		
		_UpdateConstants();

		_ComputeDispatchCounts();
	}

	state = shouldBuildMemPool ? EffectDrawerState::WantUpdateResources : EffectDrawerState::Ready;

	if (debugInfo.passInfos.empty()) {
		uint32_t passCount = (uint32_t)_drawInfo->passes.size();
		debugInfo.passInfos.resize(passCount);
		for (uint32_t i = 0; i < passCount; ++i) {
			EffectPassInfo& passInfo = debugInfo.passInfos[i];
			const ShaderEffectPassDesc& passDesc = _drawInfo->passes[i];

			passInfo.desc = passDesc.desc;

			uint32_t outputCount = (uint32_t)passDesc.outputs.size();
			passInfo.outputs.resize(outputCount);
			for (uint32_t j = 0; j < outputCount; ++j) {
				uint32_t output = passDesc.outputs[j];

				if (output == 1) {
					passInfo.outputs[j] = "OUTPUT";
				} else {
					passInfo.outputs[j] = _drawInfo->textures[size_t(output - 2)].name;
				}
			}
		}
	}
	
	return S_OK;
}

// 每个通道的输出纹理都重新申请，同时当某个纹理的内容不再被需要就将它释放。每次重新申请可能
// 会获得不同的偏移量，这意味着一个 ShaderEffectTextureDesc 可能对应多个纹理资源。
// 示例:
// 通道 1: INPUT       -> tex1   | 申请 tex1
// 通道 2: INPUT, tex1 -> tex2   | 申请 tex2，释放 INPUT (不再使用) 和 tex1 (不再读取)
// 通道 3: tex2        -> tex1   | 申请 tex1，倒数第二个通道无需释放
// 通道 4: tex1        -> OUTPUT | 最后一个通道无需申请和释放
void ShaderEffectDrawer::BuildMemPool(EffectsMemPoolBuilder& memPoolBuilder) noexcept {
	assert(_error == _Error::NoError && _drawInfo);

	if (!_hasNonPersistentResource) {
		return;
	}

	uint32_t passCount = (uint32_t)_drawInfo->passes.size();
	uint32_t textureCount = (uint32_t)_drawInfo->textures.size();
	if (passCount == 1 || textureCount == 0) {
		return;
	}

	ID3D12Device5* device = _d3d12Context->GetDevice();
	SmallVector<D3D12_RESOURCE_ALLOCATION_INFO> allocInfos(textureCount);

	// 计算每个纹理的大小和对齐
	for (uint32_t i = 0; i < textureCount; ++i) {
		DXGI_FORMAT dxgiFormat = _GetDxgiFormat(_drawInfo->textures[i].format);
		SizeU texSize = _textureSizes[i];
		
		CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			dxgiFormat, texSize.width, texSize.height, 1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		allocInfos[i] = device->GetResourceAllocationInfo(0, 1, &resDesc);
	}

	SmallVector<uint32_t> texIdxToResId(textureCount, UINT_MAX);

	// 检查是否所有通道都不读取 INPUT
	{
		bool canDiscardInput = true;
		for (uint32_t i = 0; i < passCount; ++i) {
			if (Contains(_drawInfo->passes[i].inputs, 0u)) {
				canDiscardInput = false;
				break;
			}
		}
		if (canDiscardInput) {
			memPoolBuilder.DiscardInput();
		}
	}
	
	for (uint32_t i = 0;; ++i) {
		const ShaderEffectPassDesc& passDesc = _drawInfo->passes[i];

		// 分配输出纹理内存
		for (uint32_t output : passDesc.outputs) {
			assert(output >= 2);
			uint32_t texIdx = output - 2;
			texIdxToResId[texIdx] = memPoolBuilder.AllocResource(allocInfos[texIdx]);
		}

		// 最后一个通道只会输出到 OUTPUT，因此无需分配和释放纹理。倒数第二个通道不需要释放纹理。
		if (i + 2 == passCount) {
			break;
		}

		// 如果之后的通道不读取当前输入纹理则可以释放。注意应在分配输出纹理后释放，否则输入
		// 和输出内存可能重叠。
		for (uint32_t input : passDesc.inputs) {
			assert(input != 1);

			// 从文件读取的纹理不能释放内存
			if (input >= 2 && !_drawInfo->textures[(size_t)input - 2].source.empty()) {
				continue;
			}

			// 之后的通道以当前纹理为输入则不能释放，为输出则可以释放。如果之后的通道不使用
			// 当前纹理也可以释放。
			bool canDiscard = true;
			for (uint32_t j = i + 1; j < passCount; ++j) {
				const ShaderEffectPassDesc& passDesc1 = _drawInfo->passes[j];

				if (Contains(passDesc1.inputs, input)) {
					canDiscard = false;
					break;
				}

				if (Contains(passDesc1.outputs, input)) {
					break;
				}
			}
			if (canDiscard) {
				if (input == 0) {
					memPoolBuilder.DiscardInput();
				} else {
					assert(texIdxToResId[(size_t)input - 2] != UINT_MAX);
					memPoolBuilder.DiscardResource(texIdxToResId[(size_t)input - 2]);
				}
			}
		}
	}
}

HRESULT ShaderEffectDrawer::CreateResourcesInMemPool(
	D3D12MA::Allocation* textureAllocation,
	D3D12MA::Allocation* /*bufferAllocation*/,
	std::span<const uint64_t> resourceOffsets
) noexcept {
	assert(_error == _Error::NoError && _drawInfo);

	if (!_hasNonPersistentResource) {
		return S_OK;
	}

	// 退休旧资源
	{
		auto& gpuResourceMaanger = _d3d12Context->GetGpuResourceManager();

		const uint32_t texResCount = (uint32_t)_textureResourceDatas.size();
		for (uint32_t i = _persistentResourceCount; i < texResCount; ++i) {
			gpuResourceMaanger.RetireResource(std::move(_textureResourceDatas[i].resource), false);
		}
		_textureResourceDatas.resize(_persistentResourceCount);

		if (_descriptorBaseOffset != UINT_MAX) {
			gpuResourceMaanger.RetireDescriptors(
				_descriptorBaseOffset, _descriptorCount, false, false);
			_descriptorBaseOffset = UINT_MAX;
		}
	}
	
	HRESULT hr = _CreateNonPersistentResources(textureAllocation, resourceOffsets);
	if (FAILED(hr)) {
		Logger::Get().ComError("_CreateNonPersistentResources 失败", hr);
		return hr;
	}

	hr = _CreateDescritors();
	if (FAILED(hr)) {
		Logger::Get().ComError("_CreateDescritors 失败", hr);
		return hr;
	}

	return S_OK;
}

HRESULT ShaderEffectDrawer::Draw(
	ComputeContext& computeContext,
	uint32_t inputSrvOffset,
	uint32_t outputUavOffset,
	ID3D12Resource* aliasedOutput,
	EffectDrawerExportData& exportData
) noexcept {
	assert(_error == _Error::NoError && _drawInfo);
	// 不会要求导出最后一个通道的输出
	assert(exportData.passIdx == -1 ||
		(exportData.passIdx >= 0 && exportData.passIdx + 1 < _passDatas.size() &&
			exportData.outputIdx < _drawInfo->passes[exportData.passIdx].outputs.size()));

	if (_shouldUpdateConstantBuffer) {
		_shouldUpdateConstantBuffer = false;

		computeContext.CopyBufferRegion(
			_constantBufferAllocation->GetResource(), 0,
			_constantUploadBufferAllocation->GetResource(), 0,
			_constantsDataSize
		);

		computeContext.InsertTransitionBarrier(
			_constantBufferAllocation->GetResource(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
		);
	}

	if (!_textureSourceDatas.empty()) {
		auto& gpuResourceManager = _d3d12Context->GetGpuResourceManager();

		for (_TextureSourceData& sourceData : _textureSourceDatas) {
			_TextureResourceData& texResData = _textureResourceDatas[sourceData.targetTexResIdx];
			assert(texResData.state == D3D12_RESOURCE_STATE_COPY_DEST);

			const ShaderEffectTextureDesc& effectTexDesc = _drawInfo->textures[sourceData.targetTexResIdx];
			DXGI_FORMAT format = EFFECT_TEXTURE_FORMAT_PROPS[(uint32_t)effectTexDesc.format].dxgiFormat;

			CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
				format, sourceData.textureSize.width, sourceData.textureSize.height, 1, 1);

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureLayout;
			_d3d12Context->GetDevice()->GetCopyableFootprints(&texDesc, 0, 1, 0,
				&textureLayout, nullptr, nullptr, nullptr);

			computeContext.CopyTextureRegion(
				CD3DX12_TEXTURE_COPY_LOCATION(texResData.resource.get()),
				0,
				0,
				CD3DX12_TEXTURE_COPY_LOCATION(sourceData.uploadBufferAllocation->GetResource(), textureLayout)
			);

			gpuResourceManager.RetireResource(std::move(sourceData.uploadBufferAllocation), false);
		}

		_textureSourceDatas.clear();
	}

	const uint32_t passCount = (uint32_t)_passDatas.size();

	for (uint32_t passIdx = 0; passIdx < passCount; ++passIdx) {
		const _PassData& curPassData = _passDatas[passIdx];
		const ShaderEffectPassDesc& curPassDesc = _drawInfo->passes[passIdx];
		const uint64_t inputCount = curPassData.inputTexResIdxes.size();
		const uint64_t outputCount = curPassData.outputTexResIdxes.size();

		computeContext.SetPipelineState(curPassData.pso.get());
		computeContext.SetRootSignature(curPassData.rootSignature.get());

		// 合并状态转换
		for (size_t i = 0; i < inputCount; ++i) {
			uint32_t inputTexResIdx = curPassData.inputTexResIdxes[i];

			// INPUT 是 UINT_MAX
			if (inputTexResIdx == UINT_MAX) {
				assert(curPassDesc.inputs[i] == 0);
				continue;
			}

			_TextureResourceData& texResData = _textureResourceDatas[inputTexResIdx];

			D3D12_RESOURCE_STATES oldState =
				std::exchange(texResData.state, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			if (oldState != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {
				computeContext.InsertTransitionBarrier(
					texResData.resource.get(),
					oldState,
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
				);
			}
		}

		for (size_t i = 0; i < outputCount; ++i) {
			uint32_t outputTexResIdx = curPassData.outputTexResIdxes[i];

			// OUTPUT 是 UINT_MAX
			if (outputTexResIdx == UINT_MAX) {
				assert(curPassDesc.outputs[i] == 1);

				// 由于声明了 SupportOutputAliasing，我们负责为 OUTPUT 插入别名屏障
				if (aliasedOutput) {
					computeContext.InsertAliasingBarrier(nullptr, aliasedOutput);
				}

				continue;
			}

			_TextureResourceData& texResData = _textureResourceDatas[outputTexResIdx];

			computeContext.InsertAliasingBarrier(nullptr, texResData.resource.get());

			D3D12_RESOURCE_STATES oldState =
				std::exchange(texResData.state, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			if (oldState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
				computeContext.InsertTransitionBarrier(
					texResData.resource.get(),
					oldState,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS
				);
			}
		}

		computeContext.SetRootConstantBufferView(
			0, _constantBufferAllocation->GetResource()->GetGPUVirtualAddress());

		uint32_t rootParameterIndex = 1;

		if (!curPassDesc.inputs.empty() && curPassDesc.inputs[0] == 0) {
			computeContext.SetRootDescriptorTable(rootParameterIndex++, inputSrvOffset);
		}

		if (curPassDesc.outputs[0] == 1) {
			computeContext.SetRootDescriptorTable(rootParameterIndex++, outputUavOffset);
		}

		// 不需要额外的描述符时 passData.descriptorBaseOffset 为 UINT_MAX
		if (curPassData.descriptorBaseOffset != UINT_MAX) {
			computeContext.SetRootDescriptorTable(rootParameterIndex++, curPassData.descriptorBaseOffset);
		}

		computeContext.Dispatch(curPassData.dispatchCount.first, curPassData.dispatchCount.second);

		// 导出输出纹理
		if ((int)passIdx == exportData.passIdx) {
			HRESULT hr = _Export(exportData, computeContext, passIdx);
			if (FAILED(hr)) {
				Logger::Get().ComError("_Export 失败", hr);
				return hr;
			}
		}
	}

	return S_OK;
}

HRESULT ShaderEffectDrawer::_InitializeCommonResources() noexcept {
	if (!_passDatas.empty()) {
		return S_OK;
	}

	_textureSizes.resize(_drawInfo->textures.size());
	_passDatas.resize(_drawInfo->passes.size());

	HRESULT hr = _CreatePersistentResources();
	if (FAILED(hr)) {
		Logger::Get().ComError("_CreatePersistentResources 失败", hr);
		return hr;
	} else if (hr == S_FALSE) {
		return S_FALSE;
	}

	{
		uint64_t passCount = _drawInfo->passes.size();
		uint64_t textureCount = _drawInfo->textures.size();
		SmallVector<uint32_t> texIdxToTexResIdx(textureCount, UINT_MAX);

		uint32_t curTexResIdx = 0;
		for (uint64_t i = 0; i < textureCount; ++i) {
			if (_drawInfo->textures[i].source.empty()) {
				_hasNonPersistentResource = true;
			} else {
				texIdxToTexResIdx[i] = curTexResIdx++;
			}
		}

		for (uint64_t i = 0; i < passCount; ++i) {
			_PassData& passData = _passDatas[i];
			const ShaderEffectPassDesc& passDesc = _drawInfo->passes[i];
			uint64_t inputCount = passDesc.inputs.size();

			passData.inputTexResIdxes.resize(inputCount, UINT_MAX);
			passData.outputTexResIdxes.resize(passDesc.outputs.size(), UINT_MAX);

			for (uint64_t j = 0; j < inputCount; ++j) {
				uint32_t input = passDesc.inputs[j];
				if (input >= 2) {
					passData.inputTexResIdxes[j] = texIdxToTexResIdx[(uint64_t)input - 2];
				}
			}
		}
	}

	// 如果无需使用显存池那么描述符布局不会变，只需创建一次
	if (!_hasNonPersistentResource) {
		hr = _CreateDescritors();
		if (FAILED(hr)) {
			Logger::Get().ComError("_CreateDescritors 失败", hr);
			return hr;
		}
	}

	hr = _CreateConstantBuffer();
	if (FAILED(hr)) {
		Logger::Get().ComError("_CreateConstantBuffer 失败", hr);
		return hr;
	}

	return S_OK;
}

// 有纹理尺寸变化时返回真
bool ShaderEffectDrawer::_ComputeNonPersistentTextureSizes() noexcept {
	uint32_t textureCount = (uint32_t)_drawInfo->textures.size();
	if (textureCount == 0) {
		return false;
	}

	std::optional<mu::Parser> exprParser;
	bool hasSizeChange = false;

	for (uint32_t i = 0; i < textureCount; ++i) {
		const ShaderEffectTextureDesc& effectTexDesc = _drawInfo->textures[i];

		// 从文件读取的纹理在 _CreatePersistentResources 中获取尺寸
		if (!effectTexDesc.source.empty()) {
			continue;
		}

		if (!exprParser.has_value()) {
			exprParser.emplace();
			exprParser->DefineConst("INPUT_WIDTH", _inputSize.width);
			exprParser->DefineConst("INPUT_HEIGHT", _inputSize.height);
			exprParser->DefineConst("OUTPUT_WIDTH", _outputSize.width);
			exprParser->DefineConst("OUTPUT_HEIGHT", _outputSize.height);
		}

		SizeU texSize{};
		try {
			exprParser->SetExpr(effectTexDesc.widthExpr);
			texSize.width = (uint32_t)std::max(0L, std::lround(exprParser->Eval()));
			exprParser->SetExpr(effectTexDesc.heightExpr);
			texSize.height = (uint32_t)std::max(0L, std::lround(exprParser->Eval()));
		} catch (const mu::ParserError& e) {
			Logger::Get().Error(fmt::format("计算纹理 {} 尺寸失败: {}",
				effectTexDesc.name, e.GetMsg()));
		}

		if (texSize.width == 0 || texSize.width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
			texSize.height == 0 || texSize.height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
		{
			_error = _Error::InvalidTextureSize;
			Logger::Get().Error(fmt::format("纹理 {} 尺寸无效", effectTexDesc.name));
			return false;
		}

		if (std::exchange(_textureSizes[i], texSize) != texSize) {
			hasSizeChange = true;
		}
	}

	return hasSizeChange;
}

HRESULT ShaderEffectDrawer::_CreatePersistentResources() noexcept {
	uint32_t textureCount = (uint32_t)_drawInfo->textures.size();
	if (textureCount == 0) {
		_persistentResourceCount = 0;
		_persistentResourcesAllocation = nullptr;
		return S_OK;
	}

	ID3D12Device5* device = _d3d12Context->GetDevice();
	D3D12MA::Allocator* allocator = _d3d12Context->GetGpuResourceManager().GetAllocator();

	struct PersistentResourceInfo {
		CD3DX12_RESOURCE_DESC resDesc;
		uint64_t offset;
	};
	SmallVector<PersistentResourceInfo> persistentResourceInfos;
	uint64_t persistentAllocationSize = 0;
	uint64_t persistentAllocationAlignment = 1;

	for (uint32_t i = 0; i < textureCount; ++i) {
		const ShaderEffectTextureDesc& effectTexDesc = _drawInfo->textures[i];

		if (effectTexDesc.source.empty()) {
			continue;
		}

		auto& gpuResourceManager = _d3d12Context->GetGpuResourceManager();
		DXGI_FORMAT dxgiFormat = EFFECT_TEXTURE_FORMAT_PROPS[(uint32_t)effectTexDesc.format].dxgiFormat;

		size_t delimPos = _effectInfo->name.find_last_of('\\');
		std::wstring texPath = StrHelper::UTF8ToUTF16(delimPos == std::string::npos ?
			effectTexDesc.source : StrHelper::Concat(
				std::string_view(_effectInfo->name.c_str(), delimPos + 1), effectTexDesc.source));

		_TextureSourceData& sourceData = _textureSourceDatas.emplace_back();
		
		// 可能会把 dxgiFormat 修改为 sRGB
		HRESULT hr = TextureHelper::LoadFromFile(
			AppFolderManager::Get().GetBuiltInShaderEffectsDir() / texPath,
			gpuResourceManager,
			dxgiFormat,
			sourceData.textureSize,
			sourceData.uploadBufferAllocation.put()
		);
		if (FAILED(hr)) {
			Logger::Get().ComError(fmt::format("加载纹理 {} 数据失败", effectTexDesc.name), hr);
			_error = _Error::InvalidTextureSource;
			return S_FALSE;
		}

		sourceData.targetTexResIdx = (uint32_t)persistentResourceInfos.size();

		PersistentResourceInfo& prInfo = persistentResourceInfos.emplace_back();
		prInfo.resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			dxgiFormat, sourceData.textureSize.width, sourceData.textureSize.height, 1, 1);

		D3D12_RESOURCE_ALLOCATION_INFO allocInfo =
			device->GetResourceAllocationInfo(0, 1, &prInfo.resDesc);

		prInfo.offset = DirectXHelper::Align(persistentAllocationSize, allocInfo.Alignment);
		persistentAllocationSize = prInfo.offset + allocInfo.SizeInBytes;
		persistentAllocationAlignment = std::max(persistentAllocationAlignment, allocInfo.Alignment);

		_textureSizes[i] = sourceData.textureSize;
	}

	_persistentResourceCount = (uint32_t)persistentResourceInfos.size();

	if (_persistentResourceCount == 0) {
		_persistentResourcesAllocation = nullptr;
		return S_OK;
	}

	D3D12MA::CALLOCATION_DESC allocDesc(
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12MA::ALLOCATION_FLAG_NONE,
		nullptr,
		D3D12MA_RECOMMENDED_HEAP_FLAGS | D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES
	);

	// Allocator::AllocateMemory 要求 SizeInBytes 是 64KiB 的整数倍
	D3D12_RESOURCE_ALLOCATION_INFO blockAllocInfo = {
		.SizeInBytes = DirectXHelper::Align(persistentAllocationSize, 64ull << 10),
		.Alignment = persistentAllocationAlignment
	};

	HRESULT hr = allocator->AllocateMemory(
		&allocDesc, &blockAllocInfo, _persistentResourcesAllocation.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("D3D12MA::Allocator::AllocateMemory 失败", hr);
		return hr;
	}

	uint32_t persistentResourceCount = (uint32_t)persistentResourceInfos.size();
	// 持久保存的资源放在最前
	_textureResourceDatas.resize(persistentResourceCount);

	for (uint32_t i = 0; i < persistentResourceCount; ++i) {
		const PersistentResourceInfo& prInfo = persistentResourceInfos[i];
		_TextureResourceData& texResData = _textureResourceDatas[i];

		hr = allocator->CreateAliasingResource(
			_persistentResourcesAllocation.get(),
			prInfo.offset,
			&prInfo.resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&texResData.resource)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("D3D12MA::Allocator::CreateAliasingResource 失败", hr);
			return hr;
		}

		texResData.state = D3D12_RESOURCE_STATE_COPY_DEST;
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
	assert(_constantBufferAllocation);

	using Constant32 = DirectXHelper::Constant32;
	// SmallVector<Constant32> 中数据地址偏移量是 16 字节，alignas(16) 使 SmallVector
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
			SizeU textureSize = output == 1 ? _outputSize : _textureSizes[size_t(output - 2)];
			constants.emplace_back(Constant32::Float(1.0f / textureSize.width));
			constants.emplace_back(Constant32::Float(1.0f / textureSize.height));
		}
	}

	// WCG/HDR 参数
	if (bool(_effectInfo->flags & EffectFlags::SupportAdvancedColor) &&
		_acKind != winrt::AdvancedColorKind::StandardDynamicRange)
	{
		constants.emplace_back(Constant32::Float(_maxLuminance));
		constants.emplace_back(Constant32::Float(_sdrWhiteLevel));
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
	assert(_constantsDataSize <= _constantUploadBufferAllocation->GetResource()->GetDesc().Width);
	std::memcpy(_constantUploadBufferData, constants.data(), _constantsDataSize);

	_shouldUpdateConstantBuffer = true;
}

HRESULT ShaderEffectDrawer::_CreatePSOs() noexcept {
	ID3D12Device5* device = _d3d12Context->GetDevice();

	const uint32_t passCount = (uint32_t)_drawInfo->passes.size();

	for (uint32_t passIdx = 0; passIdx < passCount; ++passIdx) {
		_PassData& curPassData = _passDatas[passIdx];
		const ShaderEffectPassDesc& curPassDesc = _drawInfo->passes[passIdx];

		winrt::com_ptr<ID3DBlob> signature;

		std::array<D3D12_ROOT_PARAMETER1, 4> rootParams;
		uint32_t curRootParamIdx = 0;
		std::array<D3D12_DESCRIPTOR_RANGE1, 4> descriptorRanges;
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

	return S_OK;
}

HRESULT ShaderEffectDrawer::_CreateConstantBuffer() noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();
	auto& gpuResourceManager = _d3d12Context->GetGpuResourceManager();

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

	uint32_t uploadBufferSize = paramCount * 4;

	HRESULT hr = gpuResourceManager.CreateBuffer(
		D3D12_HEAP_TYPE_UPLOAD,
		uploadBufferSize,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		_constantUploadBufferAllocation.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("GpuResourceManager::CreateBuffer 失败", hr);
		return hr;
	}

	hr = gpuResourceManager.CreateBuffer(
		D3D12_HEAP_TYPE_DEFAULT,
		DirectXHelper::Align(uploadBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT),
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_COMMON,
		_constantBufferAllocation.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("GpuResourceManager::CreateBuffer 失败", hr);
		return hr;
	}

	// 无需解除映射
	D3D12_RANGE readRange{};
	hr = _constantUploadBufferAllocation->GetResource()->Map(
		0, &readRange, &_constantUploadBufferData);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12Resource::Map 失败", hr);
		return hr;
	}

	return S_OK;
}

void ShaderEffectDrawer::_ComputeDispatchCounts() noexcept {
	uint32_t passCount = (uint32_t)_drawInfo->passes.size();

	for (uint32_t passIdx = 0; passIdx < passCount; ++passIdx) {
		const ShaderEffectPassDesc& curPassDesc = _drawInfo->passes[passIdx];

		SizeU outputSize;
		if (curPassDesc.outputs[0] == 1) {
			outputSize = _outputSize;
		} else {
			outputSize = _textureSizes[size_t(curPassDesc.outputs[0] - 2)];
		}

		_passDatas[passIdx].dispatchCount = {
			(outputSize.width + curPassDesc.blockSize.width - 1) / curPassDesc.blockSize.width,
			(outputSize.height + curPassDesc.blockSize.height - 1) / curPassDesc.blockSize.height
		};
	}
}

HRESULT ShaderEffectDrawer::_CreateNonPersistentResources(
	D3D12MA::Allocation* textureAllocation,
	std::span<const uint64_t> resourceOffsets
) noexcept {
	D3D12MA::Allocator* allocator = _d3d12Context->GetGpuResourceManager().GetAllocator();
	const uint32_t passCount = (uint32_t)_drawInfo->passes.size();
	const uint32_t textureCount = (uint32_t)_drawInfo->textures.size();

	// (texIdx, offset) -> texResIdx
	// 用于确保纹理名和偏移量相同时不会重复创建资源
	phmap::flat_hash_map<std::pair<uint32_t, uint64_t>, uint32_t> texResMap;
	SmallVector<uint32_t> texIdxToTexResIdx(textureCount, UINT_MAX);
	uint32_t curOffsetIdx = 0;

	// _textureResourceDatas 中持久保存的资源放在最前
	_textureResourceDatas.resize(_persistentResourceCount);

	for (uint32_t i = 0; i < passCount; ++i) {
		_PassData& passData = _passDatas[i];
		const ShaderEffectPassDesc& passDesc = _drawInfo->passes[i];
		uint32_t inputCount = (uint32_t)passDesc.inputs.size();
		uint32_t outputCount = (uint32_t)passDesc.outputs.size();
		
		// 计算 passData.inputTexResIdxes
		for (uint32_t j = 0; j < inputCount; ++j) {
			uint32_t input = passDesc.inputs[j];
			if (input >= 2) {
				uint32_t texResIdx = texIdxToTexResIdx[(uint64_t)input - 2];
				if (texResIdx != UINT_MAX) {
					passData.inputTexResIdxes[j] = texResIdx;
				}
			}
		}

		if (i + 1 == passCount) {
			continue;
		}

		// 计算 passData.outputTexResIdxes 和创建纹理资源
		for (uint32_t j = 0; j < outputCount; ++j) {
			assert(passDesc.outputs[j] >= 2);
			uint32_t texIdx = passDesc.outputs[j] - 2;
			uint64_t resourceOffset = resourceOffsets[curOffsetIdx++];

			// 确保不在同一个偏移量处重复创建资源
			{
				auto it = texResMap.find(std::make_pair(texIdx, resourceOffset));
				if (it != texResMap.end()) {
					passData.outputTexResIdxes[j] = it->second;
					continue;
				}

				uint32_t texResIdx = (uint32_t)_textureResourceDatas.size();
				texResMap.emplace(std::make_pair(texIdx, resourceOffset), texResIdx);

				texIdxToTexResIdx[texIdx] = texResIdx;
				passData.outputTexResIdxes[j] = texResIdx;
			}

			_TextureResourceData& texResData = _textureResourceDatas.emplace_back();
			DXGI_FORMAT dxgiFormat = _GetDxgiFormat(_drawInfo->textures[texIdx].format);
			SizeU texSize = _textureSizes[texIdx];

			CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
				dxgiFormat, texSize.width, texSize.height, 1, 1, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			HRESULT hr = allocator->CreateAliasingResource(
				textureAllocation,
				resourceOffset,
				&texDesc,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				IID_PPV_ARGS(&texResData.resource)
			);
			if (FAILED(hr)) {
				Logger::Get().ComError("D3D12MA::Allocator::CreateAliasingResource 失败", hr);
				return hr;
			}

			texResData.state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}
	}

	return S_OK;
}

void ShaderEffectDrawer::_ComputeDescriptorLayout(
	SmallVectorImpl<std::pair<uint32_t, DXGI_FORMAT>>& descriptorLayout,
	SmallVectorImpl<uint32_t>& passDescriptorOffsets
) const noexcept {
	const uint32_t texResCount = (uint32_t)_textureResourceDatas.size();
	const uint32_t passCount = (uint32_t)_passDatas.size();
	// [(通道序号, 需要的描述符数量)]
	SmallVector<std::pair<uint32_t, uint32_t>> descriptorCounts;

	// 计算每个通道需要的描述符数量
	for (uint32_t passIdx = 0; passIdx < passCount; ++passIdx) {
		const _PassData& passData = _passDatas[passIdx];

		uint32_t curPassDescriptorCount =
			(uint32_t)passData.inputTexResIdxes.size() +
			(uint32_t)passData.outputTexResIdxes.size();

		if (!passData.inputTexResIdxes.empty() &&
			passData.inputTexResIdxes[0] == UINT_MAX) {
			--curPassDescriptorCount;
		}

		if (passData.outputTexResIdxes[0] == UINT_MAX) {
			--curPassDescriptorCount;
		}

		if (curPassDescriptorCount != 0) {
			descriptorCounts.emplace_back(passIdx, curPassDescriptorCount);
		}
	}

	if (descriptorCounts.empty()) {
		return;
	}

	// 按需要的描述符数量从多到少排序，这可以提高复用描述符的概率
	std::sort(
		descriptorCounts.begin(),
		descriptorCounts.end(),
		[](const auto& l, const auto& r) { return l.second > r.second; }
	);

	passDescriptorOffsets.resize(passCount);

	// 计算描述符布局
	for (auto [passIdx, descriptorCount] : descriptorCounts) {
		const ShaderEffectPassDesc& passDesc = _drawInfo->passes[passIdx];
		const _PassData& passData = _passDatas[passIdx];
		const uint32_t inputCount = (uint32_t)passDesc.inputs.size();
		const uint32_t outputCount = (uint32_t)passDesc.outputs.size();

		SmallVector<std::pair<uint32_t, DXGI_FORMAT>> curPassDescriptorLayout;
		curPassDescriptorLayout.reserve(inputCount + outputCount);

		for (uint32_t i = 0; i < inputCount; ++i) {
			uint32_t input = passDesc.inputs[i];
			if (input < 2) {
				continue;
			}

			curPassDescriptorLayout.emplace_back(
				passData.inputTexResIdxes[i],
				_GetDxgiFormat(_drawInfo->textures[(uint64_t)input - 2].format)
			);
		}

		for (uint32_t i = 0; i < outputCount; ++i) {
			uint32_t output = passDesc.outputs[i];
			if (output < 2) {
				continue;
			}

			// 为了和 SRV 做区分，UAV 的索引加上纹理资源总数
			curPassDescriptorLayout.emplace_back(
				passData.outputTexResIdxes[i] + texResCount,
				_GetDxgiFormat(_drawInfo->textures[(uint64_t)output - 2].format)
			);
		}

		assert(curPassDescriptorLayout.size() == descriptorCount);

		// 寻找可以复用的区域
		auto it = std::search(
			descriptorLayout.begin(),
			descriptorLayout.end(),
			curPassDescriptorLayout.begin(),
			curPassDescriptorLayout.end(),
			[](const auto& l, const auto& r) { return l.first == r.first; }
		);
		if (it == descriptorLayout.end()) {
			passDescriptorOffsets[passIdx] = (uint32_t)descriptorLayout.size();

			descriptorLayout.insert(
				descriptorLayout.end(),
				curPassDescriptorLayout.begin(),
				curPassDescriptorLayout.end()
			);
		} else {
			passDescriptorOffsets[passIdx] = uint32_t(it - descriptorLayout.begin());
		}
	}
}

HRESULT ShaderEffectDrawer::_CreateDescritors() noexcept {
	// [(纹理资源索引, 纹理格式)]
	// UAV 的索引加上纹理资源总数以和 SRV 区分。
	SmallVector<std::pair<uint32_t, DXGI_FORMAT>> descriptorLayout;
	SmallVector<uint32_t> passDescriptorOffsets;
	_ComputeDescriptorLayout(descriptorLayout, passDescriptorOffsets);

	if (descriptorLayout.empty()) {
		// 不需要额外的描述符
		for (_PassData& passData : _passDatas) {
			passData.descriptorBaseOffset = UINT_MAX;
		}

		return S_OK;
	}

	// 申请描述符空间
	auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();
	assert(_descriptorBaseOffset == UINT_MAX);
	_descriptorCount = (uint32_t)descriptorLayout.size();
	HRESULT hr = descriptorHeap.Alloc(_descriptorCount, _descriptorBaseOffset);
	if (FAILED(hr)) {
		Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
		return hr;
	}
	
	// 计算 passData.descriptorBaseOffset
	uint64_t passCount = _passDatas.size();
	for (uint64_t i = 0; i < passCount; ++i) {
		_passDatas[i].descriptorBaseOffset = _descriptorBaseOffset + passDescriptorOffsets[i];
	}

	ID3D12Device5* device = _d3d12Context->GetDevice();
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptorHeap.GetCpuHandle(_descriptorBaseOffset));
	uint32_t descriptorSize = descriptorHeap.GetDescriptorSize();
	const uint32_t texResCount = (uint32_t)_textureResourceDatas.size();

	// 创建描述符
	for (auto [texResIdx, dxgiFormat] : descriptorLayout) {
		// 为了和 SRV 做区分，UAV 的索引加上了纹理资源总数
		bool isUAV = texResIdx >= texResCount;
		if (isUAV) {
			texResIdx -= texResCount;
		}

		ID3D12Resource* texture = _textureResourceDatas[texResIdx].resource.get();

		if (isUAV) {
			CD3DX12_UNORDERED_ACCESS_VIEW_DESC uavDesc =
				CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2D(dxgiFormat);
			device->CreateUnorderedAccessView(texture, nullptr, &uavDesc, cpuHandle);
		} else {
			CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc =
				CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(dxgiFormat, 1);
			device->CreateShaderResourceView(texture, &srvDesc, cpuHandle);
		}

		cpuHandle.Offset(descriptorSize);
	}

	return S_OK;
}

DXGI_FORMAT ShaderEffectDrawer::_GetDxgiFormat(EffectTextureFormat texFormat) const noexcept {
	if (texFormat == EffectTextureFormat::COLOR_SPACE_ADAPTIVE) {
		if (_acKind == winrt::AdvancedColorKind::StandardDynamicRange) {
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		} else {
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		}
	} else {
		return EFFECT_TEXTURE_FORMAT_PROPS[(uint32_t)texFormat].dxgiFormat;
	}
}

HRESULT ShaderEffectDrawer::_Export(
	EffectDrawerExportData& exportData,
	ComputeContext& computeContext,
	uint32_t passIdx
) noexcept {
	auto& gpuResourceManager = _d3d12Context->GetGpuResourceManager();
	const _PassData& passData = _passDatas[passIdx];
	const ShaderEffectPassDesc& passDesc = _drawInfo->passes[passIdx];

	assert(passDesc.outputs[exportData.outputIdx] >= 2);
	uint32_t exportTexIdx = passDesc.outputs[exportData.outputIdx] - 2;
	uint32_t exportTexResIdx = passData.outputTexResIdxes[exportData.outputIdx];
	// 不会导出最后一个通道，因此只会导出中间纹理
	assert(exportTexResIdx != UINT_MAX);

	_TextureResourceData& exportTexResData = _textureResourceDatas[exportTexResIdx];

	exportData.textureSize = _textureSizes[exportTexIdx];

	exportData.textureFormat = _drawInfo->textures[exportTexIdx].format;
	assert(exportData.textureFormat != EffectTextureFormat::UNKNOWN);
	if (exportData.textureFormat == EffectTextureFormat::COLOR_SPACE_ADAPTIVE) {
		if (_acKind == winrt::AdvancedColorKind::StandardDynamicRange) {
			exportData.textureFormat = EffectTextureFormat::R10G10B10A2_UNORM;
		} else {
			exportData.textureFormat = EffectTextureFormat::R16G16B16A16_FLOAT;
		}
	}

	const EffectTextureFormatProps& textureFormatProps =
		EFFECT_TEXTURE_FORMAT_PROPS[(uint32_t)exportData.textureFormat];

	uint32_t rowSize;
	size_t bufferSize;
	DirectXHelper::ComputeBufferLayout(exportData.textureSize,
		textureFormatProps.texelSize, rowSize, exportData.rowPitch, bufferSize);

	HRESULT hr = gpuResourceManager.CreateBuffer(
		D3D12_HEAP_TYPE_READBACK,
		bufferSize,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_COPY_DEST,
		exportData.readbackBufferAllocation.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("GpuResourceManager::CreateBuffer 失败", hr);
		return hr;
	}

	computeContext.InsertTransitionBarrier(
		exportTexResData.resource.get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE
	);
	exportTexResData.state = D3D12_RESOURCE_STATE_COPY_SOURCE;

	computeContext.CopyTextureRegion(
		CD3DX12_TEXTURE_COPY_LOCATION(
			exportData.readbackBufferAllocation->GetResource(),
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT{
				.Footprint = {
					.Format = textureFormatProps.dxgiFormat,
					.Width = exportData.textureSize.width,
					.Height = exportData.textureSize.height,
					.Depth = 1,
					.RowPitch = exportData.rowPitch
				}
			}
		),
		0,
		0,
		CD3DX12_TEXTURE_COPY_LOCATION(exportTexResData.resource.get())
	);

	return S_OK;
}

}
