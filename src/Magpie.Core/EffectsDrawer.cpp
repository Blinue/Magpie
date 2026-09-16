#include "pch.h"
#include "EffectsDrawer.h"
#include "CatmullRomDrawer.h"
#include "CommandContext.h"
#include "D3D12Context.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "ShaderEffectDrawer.h"
#include "EffectInfo.h"
#include "DescriptorHeap.h"

namespace Magpie {

EffectsDrawer::~EffectsDrawer() noexcept {
#ifdef _DEBUG
	if (_descriptorBaseOffset != std::numeric_limits<uint32_t>::max()) {
		_d3d12Context->GetDescriptorHeap().Free(_descriptorBaseOffset, _CalcDescriptorCount());
	}
#endif
}

static SizeU CalcOutputSize(
	uint32_t scaleFactor,
	SizeU inputSize,
	SizeU rendererSize,
	const EffectOption& effectOption
) noexcept {
	if (scaleFactor != 0) {
		return SizeU{ inputSize.width * scaleFactor, inputSize.height * scaleFactor };
	}

	// 支持自由缩放
	switch (effectOption.scalingType) {
	case ScalingType::Normal:
	{
		return SizeU{
			(uint32_t)std::lround(inputSize.width * effectOption.scale.first),
			(uint32_t)std::lround(inputSize.height * effectOption.scale.second)
		};
	}
	case ScalingType::Absolute:
	{
		return SizeU{
			(uint32_t)std::lround(effectOption.scale.first),
			(uint32_t)std::lround(effectOption.scale.second)
		};
	}
	case ScalingType::Fit:
	{
		// 窗口模式缩放时将缩放比例为 1 的 Fit 视为 Fill。此时缩放确保是等比例的，但由于舍入
		// 可能存在一个像素的误差。考虑长 100 高 50 的矩形窗口，长调整到 101 时高将四舍五入到
		// 51，再将长调整到 102 高仍是 51，Fit 的计算方式会使这两次调整中有一次存在黑边。
		bool treatFitAsFill = ScalingWindow::Get().Options().IsWindowedMode() &&
			IsApprox(effectOption.scale.first, 1.0f) &&
			IsApprox(effectOption.scale.second, 1.0f);

		if (!treatFitAsFill) {
			float fillScale = std::min(
				float(rendererSize.width) / inputSize.width,
				float(rendererSize.height) / inputSize.height
			);
			return SizeU{
				(uint32_t)std::lround(inputSize.width * fillScale * effectOption.scale.first),
				(uint32_t)std::lround(inputSize.height * fillScale * effectOption.scale.second)
			};
		}
		[[fallthrough]];
	}
	default:
		assert(effectOption.scalingType == ScalingType::Fit ||
			effectOption.scalingType == ScalingType::Fill);
		return rendererSize;
	}
}

bool EffectsDrawer::Initialize(
	D3D12Context& d3d12Context,
	const ColorInfo& colorInfo,
	SizeU inputSize,
	SizeU rendererSize,
	SizeU& outputSize
) noexcept {
	_d3d12Context = &d3d12Context;
	_colorInfo = colorInfo;
	_inputSize = inputSize;
	_rendererSize = rendererSize;

	ID3D12Device5* device = d3d12Context.GetDevice();
	const ScalingOptions& options = ScalingWindow::Get().Options();

	uint32_t effectCount = (uint32_t)options.effects.size();
	_effectDatas.resize(effectCount);

	// 效果的初始化可能是异步的，因此尽早进行
	for (uint32_t i = 0; i < effectCount; ++i) {
		_EffectData& effectData = _effectDatas[i];
		effectData.drawer = std::make_unique<ShaderEffectDrawer>();
		effectData.effectInfo = effectData.drawer->Initialize(d3d12Context, options.effects[i]);
		if (!effectData.effectInfo) {
			Logger::Get().Error("ShaderEffectDrawer::Initialize 失败");
			return false;
		}
	}

	_UpdateEffectBindings();
	outputSize = _outputSize;

	// 创建效果的输入/输出纹理
	if (uint32_t descriptorCount = _CalcDescriptorCount()) {
		auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();
		HRESULT hr = descriptorHeap.Alloc(descriptorCount, _descriptorBaseOffset);
		if (FAILED(hr)) {
			Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
			return false;
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptorHeap.GetCpuHandle(_descriptorBaseOffset));
		const uint32_t descriptorSize = descriptorHeap.GetDescriptorSize();

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

		D3D12_HEAP_FLAGS heapFlags = _d3d12Context->IsHeapFlagCreateNotZeroedSupported() ?
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

		bool isSrgb = colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange;
		CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			isSrgb ? DXGI_FORMAT_R10G10B10A2_UNORM : DXGI_FORMAT_R16G16B16A16_FLOAT,
			0, 0, 1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		);

		CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc =
			CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(texDesc.Format, 1);
		CD3DX12_UNORDERED_ACCESS_VIEW_DESC uavDesc =
			CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2D(texDesc.Format);

		for (uint32_t i = 0; i < effectCount; ++i) {
			auto& effectData = _effectDatas[i];

			// 如果不需要缩小，最后一个效果直接写入环形缓冲，不需要创建输出纹理
			if (i == effectCount - 1 && effectData.outputSize == _outputSize) {
				break;
			}

			texDesc.Width = effectData.outputSize.width;
			texDesc.Height = effectData.outputSize.height;

			hr = device->CreateCommittedResource(
				&heapProps, heapFlags, &texDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				nullptr, IID_PPV_ARGS(&effectData.outputTexture));
			if (FAILED(hr)) {
				Logger::Get().ComError("CreateCommittedResource 失败", hr);
				return false;
			}

			device->CreateShaderResourceView(effectData.outputTexture.get(), &srvDesc, cpuHandle);
			cpuHandle.Offset(descriptorSize);

			device->CreateUnorderedAccessView(
				effectData.outputTexture.get(), nullptr, &uavDesc, cpuHandle);
			cpuHandle.Offset(descriptorSize);
		}
	}

	// CatmullRomDrawer 将在渲染时按需创建 PSO，初始化无代价
	_catmullRomDrawer.Initialize(d3d12Context);

	{
		// 每帧两个时间戳
		const uint32_t timestampCount = 2 * ScalingWindow::Get().Options().maxProducerInFlightFrames;

		D3D12_QUERY_HEAP_DESC queryHeapDesc = {
			.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
			.Count = timestampCount
		};
		HRESULT hr = device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&_queryHeap));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateQueryHeap 失败", hr);
			return false;
		}

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_READBACK);
		CD3DX12_RESOURCE_DESC bufferDesc =
			CD3DX12_RESOURCE_DESC::Buffer(timestampCount * sizeof(UINT64));
		hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&_queryResultBuffer)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommittedResource 失败", hr);
			return false;
		}

		hr = d3d12Context.GetCommandQueue()->GetTimestampFrequency(&_timestampFrequency);
		if (FAILED(hr)) {
			Logger::Get().ComError("ID3D12CommandQueue::GetTimestampFrequency 失败", hr);
			return false;
		}
	}

	return true;
}

HRESULT EffectsDrawer::Draw(
	ComputeContext& computeContext,
	uint32_t /*frameIndex*/,
	ID3D12Resource* /*inputResource*/,
	ID3D12Resource* /*outputResource*/,
	uint32_t inputSrvOffset,
	uint32_t outputUavOffset
) noexcept {
	// 获取渲染时间
	// const uint32_t queryHeapIndex = 2 * frameIndex;
	// {
	// 	CD3DX12_RANGE range(queryHeapIndex * sizeof(UINT64), (queryHeapIndex + 2) * sizeof(UINT64));
	   
	// 	void* pData;
	// 	HRESULT hr = _queryResultBuffer->Map(0, nullptr, &pData);
	// 	if (FAILED(hr)) {
	// 		Logger::Get().ComError("ID3D12Resource::Map 失败", hr);
	// 		return hr;
	// 	}
	   
	// 	UINT64* timestampes = (UINT64*)pData + queryHeapIndex;
	   
	// 	range = {};
	// 	_queryResultBuffer->Unmap(0, &range);
	// }

	//commandList->EndQuery(_queryHeap.get(), D3D12_QUERY_TYPE_TIMESTAMP, queryHeapIndex);

	const uint32_t effectCount = (uint32_t)_effectDatas.size();
	// 如果多个连续的效果都不能渲染，则合并为一个 CatmullRom
	uint32_t catmullRomStartIdx = std::numeric_limits<uint32_t>::max();

	for (uint32_t effectIdx = 0; effectIdx < effectCount; ++effectIdx) {
		EffectDrawerState state;
		std::string msg;
		HRESULT hr = _effectDatas[effectIdx].drawer->Update(state, msg);
		if (FAILED(hr)) {
			Logger::Get().ComError("ShaderEffectDrawer::Update 失败", hr);
			return hr;
		}

		if (state != EffectDrawerState::Ready) {
			if (catmullRomStartIdx == std::numeric_limits<uint32_t>::max()) {
				catmullRomStartIdx = effectIdx;
			}
			continue;
		}

		if (catmullRomStartIdx != std::numeric_limits<uint32_t>::max()) {
			SizeU inputSize;
			uint32_t inputSrv;
			if (catmullRomStartIdx == 0) {
				inputSize = _inputSize;
				inputSrv = inputSrvOffset;
			} else {
				uint32_t prevIdx = catmullRomStartIdx - 1;
				inputSize = _effectDatas[prevIdx].outputSize;
				inputSrv = _descriptorBaseOffset + prevIdx * 2;

				computeContext.InsertTransitionBarrier(
					_effectDatas[prevIdx].outputTexture.get(),
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
				);
			}

			computeContext.InsertTransitionBarrier(
				_effectDatas[size_t(effectIdx - 1)].outputTexture.get(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);

			_catmullRomDrawer.Draw(
				computeContext,
				inputSize,
				_effectDatas[size_t(effectIdx - 1)].outputSize,
				inputSrv,
				_descriptorBaseOffset + effectIdx * 2 - 1,
				false
			);

			catmullRomStartIdx = std::numeric_limits<uint32_t>::max();
		}

		bool writeToRingBuffer = effectIdx == effectCount - 1 &&
			_effectDatas[effectIdx].outputSize == _outputSize;

		if (effectIdx != 0) {
			computeContext.InsertTransitionBarrier(
				_effectDatas[size_t(effectIdx - 1)].outputTexture.get(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			);
		}

		if (!writeToRingBuffer) {
			computeContext.InsertTransitionBarrier(
				_effectDatas[effectIdx].outputTexture.get(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
		}

		hr = _effectDatas[effectIdx].drawer->Draw(
			computeContext,
			effectIdx == 0 ? inputSrvOffset : _descriptorBaseOffset + (effectIdx - 1) * 2,
			writeToRingBuffer ? outputUavOffset : _descriptorBaseOffset + effectIdx * 2 + 1
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("EffectDrawerBase::Draw 失败", hr);
			return hr;
		}
	}

	if (catmullRomStartIdx != std::numeric_limits<uint32_t>::max()) {
		SizeU inputSize;
		uint32_t inputSrv;
		if (catmullRomStartIdx == 0) {
			inputSize = _inputSize;
			inputSrv = inputSrvOffset;
		} else {
			uint32_t prevIdx = catmullRomStartIdx - 1;
			inputSize = _effectDatas[prevIdx].outputSize;
			inputSrv = _descriptorBaseOffset + prevIdx * 2;

			computeContext.InsertTransitionBarrier(
				_effectDatas[prevIdx].outputTexture.get(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			);
		}

		_catmullRomDrawer.Draw(
			computeContext, inputSize, _outputSize, inputSrv, outputUavOffset, false);
	} else if (_effectDatas.back().outputSize != _outputSize) {
		computeContext.InsertTransitionBarrier(
			_effectDatas.back().outputTexture.get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);

		_catmullRomDrawer.Draw(
			computeContext,
			_effectDatas.back().outputSize,
			_outputSize,
			_descriptorBaseOffset + (effectCount - 1) * 2,
			outputUavOffset,
			false
		);
	}

	// commandList->EndQuery(_queryHeap.get(), D3D12_QUERY_TYPE_TIMESTAMP, queryHeapIndex + 1);
	// commandList->ResolveQueryData(_queryHeap.get(), D3D12_QUERY_TYPE_TIMESTAMP, queryHeapIndex, 2,
	//	_queryResultBuffer.get(), queryHeapIndex * sizeof(UINT64));

	return S_OK;
}

void EffectsDrawer::OnResized(SizeU rendererSize, SizeU& outputSize) noexcept {
	_rendererSize = rendererSize;
	_UpdateEffectBindings();
	outputSize = _outputSize;
}

void EffectsDrawer::OnColorInfoChanged(const ColorInfo& colorInfo) noexcept {
	_colorInfo = colorInfo;
	_UpdateEffectBindings();
}

uint32_t EffectsDrawer::_CalcDescriptorCount() const noexcept {
	// 如果最后一个效果的缩放类型是 Fit 或 Fill 且缩放比例不大于 1，那么始终可以直接写入环形缓冲区，
	// 需要的描述符数量可以减少两个。
	// 还有更复杂的情况，如倒数第二个效果是 Fit(0.5,0.5)，最后一个效果放大一倍，也可以认为输出尺寸
	// 永远不会大于 rendererSize，不过这较为复杂，还有舍入的问题，安全起见不进行优化。
	uint32_t count = (uint32_t)_effectDatas.size() * 2;

	if (_effectDatas.back().effectInfo->scaleFactor != 0) {
		return count;
	}

	const EffectOption& effectOption = ScalingWindow::Get().Options().effects.back();
	if ((effectOption.scalingType == ScalingType::Fit || effectOption.scalingType == ScalingType::Fill) &&
		effectOption.scale.first < 1 + FLOAT_EPSILON<float> &&
		effectOption.scale.second < 1 + FLOAT_EPSILON<float>)
	{
		return count - 2;
	} else {
		return count;
	}
}

void EffectsDrawer::_UpdateEffectBindings() noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();

	_outputSize = _inputSize;
	for (uint32_t i = 0; i < _effectDatas.size(); ++i) {
		_EffectData& effectData = _effectDatas[i];
		const EffectOption& effectOption = options.effects[i];

		// outputSize 是前一个效果的输出尺寸，即当前效果的输入尺寸
		effectData.outputSize = CalcOutputSize(
			effectData.effectInfo->scaleFactor, _outputSize, _rendererSize, effectOption);

		effectData.drawer->Bind(_outputSize, effectData.outputSize, _colorInfo);

		_outputSize = effectData.outputSize;
	}

	// 如果输出尺寸比渲染区域更大则使用 CatmullRom 等比缩小，窗口模式缩放下可能要放大
	if (_outputSize != _rendererSize) {
		if (options.IsWindowedMode()) {
			// 窗口模式缩放已确保等比例，这里直接赋值以避免舍入误差
			_outputSize = _rendererSize;
		} else if (_outputSize.width > _rendererSize.width ||
			_outputSize.height > _rendererSize.height)
		{
			float scaleX = float(_rendererSize.width) / _outputSize.width;
			float scaleY = float(_rendererSize.height) / _outputSize.height;
			if (scaleX <= scaleY) {
				_outputSize.width = _rendererSize.width;
				_outputSize.height = std::lround(_outputSize.height * scaleX);
			} else {
				_outputSize.width = std::lround(_outputSize.width * scaleY);
				_outputSize.height = _rendererSize.height;
			}
		}
	}
}

}
