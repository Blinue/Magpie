#include "pch.h"
#include "EffectsDrawer.h"
#include "CatmullRomDrawer.h"
#include "CommandContext.h"
#include "D3D12Context.h"
#include "DescriptorHeap.h"
#include "EffectInfo.h"
#include "GpuResourceManager.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "ShaderEffectDrawer.h"

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
	const winrt::DispatcherQueue& dispatcher,
	const ColorInfo& colorInfo,
	SizeU inputSize,
	SizeU rendererSize,
	SizeU& outputSize
) noexcept {
	_d3d12Context = &d3d12Context;
	_dispatcher = dispatcher;
	_colorInfo = colorInfo;
	_inputSize = inputSize;
	_rendererSize = rendererSize;

	ID3D12Device5* device = d3d12Context.GetDevice();
	auto& gpuResourceManager = d3d12Context.GetGpuResourceManager();
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

		hr = _UpdateEffectOutputTextures();
		if (FAILED(hr)) {
			Logger::Get().ComError("_UpdateEffectOutputTextures 失败", hr);
			return false;
		}
	}

	// CatmullRomDrawer 将在渲染时按需创建 PSO，初始化无代价
	_catmullRomDrawer.Initialize(d3d12Context);

	_screenshotDrawer.Initialize(d3d12Context, dispatcher, colorInfo);

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

		hr = gpuResourceManager.CreateBuffer(
			D3D12_HEAP_TYPE_READBACK,
			timestampCount * sizeof(UINT64),
			D3D12_RESOURCE_STATE_COPY_DEST,
			_queryResultBufferAllocation.put()
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("GpuResourceManager::CreateBuffer 失败", hr);
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
	ID3D12Resource* outputResource,
	uint32_t inputSrvOffset,
	uint32_t outputUavOffset,
	uint64_t /*completedFenceValue*/,
	uint64_t frameFenceValue
) noexcept {
	_ExportTask exportTask = _exportTask.exchange(_ExportTask(), std::memory_order_relaxed);

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

	if (exportTask.effectIdx == -2) {
		// 导出捕获画面
		HRESULT hr = _screenshotDrawer.Draw(computeContext, inputSrvOffset, _inputSize, frameFenceValue);
		if (FAILED(hr)) {
			Logger::Get().ComError("ScreenshotDrawer::Draw 失败", hr);
			return hr;
		}
	}

	bool exportRingBuffer = exportTask.effectIdx == -1;

	const uint32_t effectCount = (uint32_t)_effectDatas.size();
	// 如果多个连续的效果都不能渲染，则合并为一个 CatmullRom
	uint32_t catmullRomStartIdx = std::numeric_limits<uint32_t>::max();

	for (uint32_t effectIdx = 0; effectIdx < effectCount; ++effectIdx) {
		_EffectData& curEffectData = _effectDatas[effectIdx];

		EffectDrawerState state;
		HRESULT hr = curEffectData.drawer->Update(state, curEffectData.debugInfo);
		if (FAILED(hr)) {
			Logger::Get().ComError(fmt::format("EffectDrawer[{}]::Update 失败", effectIdx), hr);
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
			}

			ID3D12Resource* prevOutputTexture =
				_effectDatas[size_t(effectIdx - 1)].outputTextureAllocation->GetResource();

			computeContext.InsertTransitionBarrier(
				prevOutputTexture,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);

			_catmullRomDrawer.Draw(
				computeContext,
				inputSize,
				_effectDatas[size_t(effectIdx - 1)].outputSize,
				inputSrv,
				_descriptorBaseOffset + effectIdx * 2 - 1
			);

			computeContext.InsertTransitionBarrier(
				prevOutputTexture,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			);

			catmullRomStartIdx = std::numeric_limits<uint32_t>::max();
		}

		bool writeToRingBuffer = effectIdx == effectCount - 1 &&
			curEffectData.outputSize == _outputSize;

		if (writeToRingBuffer) {
			computeContext.InsertTransitionBarrier(
				outputResource,
				D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
		} else {
			computeContext.InsertTransitionBarrier(
				curEffectData.outputTextureAllocation->GetResource(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
		}

		bool exportCurEffect = (int)effectIdx == exportTask.effectIdx;
		bool exportInternalTexture = exportCurEffect && exportTask.passIdx >= 0 &&
			exportTask.passIdx < (int)curEffectData.debugInfo.passInfos.size() - 1;

		EffectDrawerExportData exportData = {
			.passIdx = exportInternalTexture ? exportTask.passIdx : -1,
			.outputIdx = exportTask.outputIdx
		};

		hr = curEffectData.drawer->Draw(
			computeContext,
			effectIdx == 0 ? inputSrvOffset : _descriptorBaseOffset + (effectIdx - 1) * 2,
			writeToRingBuffer ? outputUavOffset : _descriptorBaseOffset + effectIdx * 2 + 1,
			exportData
		);
		if (FAILED(hr)) {
			Logger::Get().ComError(fmt::format("EffectDrawer[{}]::Draw 失败", effectIdx), hr);
			return hr;
		}

		if (exportInternalTexture) {
			_screenshotDrawer.ExportInternalTexture(std::move(exportData), frameFenceValue, effectIdx);
		}

		if (!writeToRingBuffer) {
			computeContext.InsertTransitionBarrier(
				curEffectData.outputTextureAllocation->GetResource(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			);
		}

		if (exportCurEffect && (exportTask.passIdx == -1 ||
			exportTask.passIdx == (int)curEffectData.debugInfo.passInfos.size() - 1))
		{
			// 导出当前效果的输出
			if (writeToRingBuffer) {
				exportRingBuffer = true;
			} else {
				hr = _screenshotDrawer.Draw(computeContext, _descriptorBaseOffset + effectIdx * 2,
					curEffectData.outputSize, frameFenceValue);
				if (FAILED(hr)) {
					Logger::Get().ComError("ScreenshotDrawer::Draw 失败", hr);
					return hr;
				}
			}
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
		}

		computeContext.InsertTransitionBarrier(
			outputResource,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);

		_catmullRomDrawer.Draw(
			computeContext, inputSize, _outputSize, inputSrv, outputUavOffset);
	} else if (_effectDatas.back().outputSize != _outputSize) {
		computeContext.InsertTransitionBarrier(
			outputResource,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);

		_catmullRomDrawer.Draw(
			computeContext,
			_effectDatas.back().outputSize,
			_outputSize,
			_descriptorBaseOffset + (effectCount - 1) * 2,
			outputUavOffset
		);
	}

	if (exportRingBuffer) {
		// 导出最终输出画面
		computeContext.InsertTransitionBarrier(
			outputResource,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);

		EffectTextureFormat textureFormat =
			_colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange ?
			EffectTextureFormat::R8G8B8A8_UNORM : EffectTextureFormat::R16G16B16A16_FLOAT;
		HRESULT hr = _screenshotDrawer.Draw(
			computeContext, outputResource, textureFormat, _outputSize, frameFenceValue);
		if (FAILED(hr)) {
			Logger::Get().ComError("ScreenshotDrawer::Draw 失败", hr);
			return hr;
		}

		computeContext.InsertTransitionBarrier(
			outputResource,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_COMMON
		);
	} else {
		computeContext.InsertTransitionBarrier(
			outputResource,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COMMON
		);
	}

	// commandList->EndQuery(_queryHeap.get(), D3D12_QUERY_TYPE_TIMESTAMP, queryHeapIndex + 1);
	// commandList->ResolveQueryData(_queryHeap.get(), D3D12_QUERY_TYPE_TIMESTAMP, queryHeapIndex, 2,
	//	_queryResultBuffer.get(), queryHeapIndex * sizeof(UINT64));

	return S_OK;
}

void EffectsDrawer::ExportAsync(int effectIdx, int passIdx, uint32_t outputIdx) noexcept {
	assert(effectIdx >= -2 && passIdx >= -1);

	// 只保存一个截图任务
	_exportTask.store(_ExportTask{ effectIdx, passIdx, outputIdx }, std::memory_order_relaxed);

	// 立刻渲染导出截图
	ScalingWindow::Get().RequestNewFrame(true);
}

HRESULT EffectsDrawer::OnResized(SizeU rendererSize, SizeU& outputSize) noexcept {
	_rendererSize = rendererSize;
	_UpdateEffectBindings();
	outputSize = _outputSize;

	HRESULT hr = _UpdateEffectOutputTextures();
	if (FAILED(hr)) {
		Logger::Get().ComError("_UpdateEffectOutputTextures 失败", hr);
		return hr;
	}

	return S_OK;
}

HRESULT EffectsDrawer::OnColorInfoChanged(const ColorInfo& colorInfo) noexcept {
	const bool wasSdr = _colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange;
	_colorInfo = colorInfo;

	_UpdateEffectBindings();

	bool isSdr = _colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange;
	if (isSdr != wasSdr) {
		HRESULT hr = _UpdateEffectOutputTextures();
		if (FAILED(hr)) {
			Logger::Get().ComError("_UpdateEffectOutputTextures 失败", hr);
			return hr;
		}
	}

	_screenshotDrawer.OnColorInfoChanged(colorInfo);

	return S_OK;
}

void EffectsDrawer::OnProducerFrameCompleted(uint64_t fenceValue) noexcept {
	_screenshotDrawer.OnProducerFrameCompleted(fenceValue);
}

uint32_t EffectsDrawer::_CalcDescriptorCount() const noexcept {
	// 如果最后一个效果的缩放类型是 Fit 或 Fill 且缩放比例不大于 1，那么始终可以直接写入环形缓冲区，
	// 需要的描述符数量可以减少两个。
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

	uint32_t effectCount = (uint32_t)_effectDatas.size();
	for (uint32_t i = 0; i < effectCount; ++i) {
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
			// 窗口模式缩放始终填满渲染区域
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

HRESULT EffectsDrawer::_UpdateEffectOutputTextures() noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();
	ID3D12Device5* device = _d3d12Context->GetDevice();
	auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();
	auto& gpuResourceManager = _d3d12Context->GetGpuResourceManager();

	CD3DX12_CPU_DESCRIPTOR_HANDLE baseHandle(descriptorHeap.GetCpuHandle(_descriptorBaseOffset));
	uint32_t descriptorSize = descriptorHeap.GetDescriptorSize();

	DXGI_FORMAT texFormat = _colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange ?
		DXGI_FORMAT_R10G10B10A2_UNORM : DXGI_FORMAT_R16G16B16A16_FLOAT;

	CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(texFormat, 1);
	CD3DX12_UNORDERED_ACCESS_VIEW_DESC uavDesc = CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2D(texFormat);

	uint32_t effectCount = (uint32_t)options.effects.size();
	for (uint32_t i = 0; i < effectCount; ++i) {
		auto& effectData = _effectDatas[i];

		// 如果不需要缩小，最后一个效果直接写入环形缓冲，不需要创建输出纹理
		if (i == effectCount - 1 && effectData.outputSize == _outputSize) {
			effectData.outputTextureAllocation = nullptr;
			break;
		}

		if (effectData.outputTextureAllocation) {
			D3D12_RESOURCE_DESC desc = effectData.outputTextureAllocation->GetResource()->GetDesc();
			if (desc.Width == effectData.outputSize.width &&
				desc.Height == effectData.outputSize.height &&
				desc.Format == texFormat)
			{
				continue;
			}
		}

		HRESULT hr = gpuResourceManager.CreateTexture(
			D3D12_HEAP_TYPE_DEFAULT,
			texFormat,
			effectData.outputSize,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			effectData.outputTextureAllocation.put()
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("GpuResourceManager::CreateTexture 失败", hr);
			return hr;
		}

		assert(i * 2 + 1 <= _CalcDescriptorCount());

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(baseHandle, i * 2, descriptorSize);
		device->CreateShaderResourceView(
			effectData.outputTextureAllocation->GetResource(), &srvDesc, cpuHandle);

		cpuHandle.Offset(descriptorSize);
		device->CreateUnorderedAccessView(
			effectData.outputTextureAllocation->GetResource(), nullptr, &uavDesc, cpuHandle);
	}

	return S_OK;
}

}
