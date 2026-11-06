#include "pch.h"
#include "EffectsDrawer.h"
#include "CatmullRomDrawer.h"
#include "CommandContext.h"
#include "D3D12Context.h"
#include "DescriptorHeap.h"
#include "EffectsMemPoolBuilder.h"
#include "GpuResourceManager.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "ShaderEffectDrawer.h"
#include "DirectXHelper.h"

namespace Magpie {

EffectsDrawer::~EffectsDrawer() noexcept {
#ifdef _DEBUG
	if (_descriptorBaseOffset != UINT_MAX) {
		_d3d12Context->GetDescriptorHeap().Free(_descriptorBaseOffset, _descriptorCount);
	}
#endif
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

		HRESULT hr = effectData.drawer->Initialize(d3d12Context, options.effects[i], effectData.drawerInfo);
		if (FAILED(hr)) {
			Logger::Get().ComError(fmt::format("EffectDrawer[{}]::Initialize 失败", i), hr);
			return false;
		}
	}

	_UpdateEffectBindings();
	outputSize = _outputSize;

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

static bool IsEffectReadyToDraw(EffectDrawerState state) noexcept {
	return state == EffectDrawerState::WantUpdateResources ||
		state == EffectDrawerState::Ready;
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

	const uint32_t effectCount = (uint32_t)_effectDatas.size();
	const bool isLastEffectWriteToRingBuffer = _effectDatas.back().outputSize == _outputSize;
	bool shouldBuildMemPool = false;
	SmallVector<EffectDrawerState> effectDrawerStates(effectCount);

	for (uint32_t i = 0; i < effectCount; ++i) {
		_EffectData& curEffectData = _effectDatas[i];

		EffectDrawerState& state = effectDrawerStates[i];
		HRESULT hr = curEffectData.drawer->Update(state, curEffectData.debugInfo);
		if (FAILED(hr)) {
			Logger::Get().ComError(fmt::format("EffectDrawer[{}]::Update 失败", i), hr);
			return hr;
		}

		if (!shouldBuildMemPool) {
			if (state == EffectDrawerState::WantUpdateResources) {
				shouldBuildMemPool = true;
			} else if (state == EffectDrawerState::Ready) {
				// 需确保效果的输出纹理存在。两种情况下不需要创建输出纹理:
				// 1. 直接写入环形缓冲的最后一个效果
				// 2. 当前效果和下一个效果都不能渲染

				if (i + 1 != effectCount || !isLastEffectWriteToRingBuffer) {
					shouldBuildMemPool = !curEffectData.outputTexture;
				}

				// 当前效果已经可以渲染，因此前一个效果需要输出纹理
				if (!shouldBuildMemPool && i > 0) {
					shouldBuildMemPool = !_effectDatas[(uint64_t)i - 1].outputTexture;
				}
			}
		}
	}

	if (shouldBuildMemPool) {
		HRESULT hr = _BuildMemPool(effectDrawerStates);
		if (FAILED(hr)) {
			Logger::Get().ComError("_BuildMemPool 失败", hr);
			return hr;
		}
	}

	if (exportTask.effectIdx == -2) {
		// 导出捕获画面
		HRESULT hr = _screenshotDrawer.Draw(computeContext, inputSrvOffset, _inputSize, frameFenceValue);
		if (FAILED(hr)) {
			Logger::Get().ComError("ScreenshotDrawer::Draw 失败", hr);
			return hr;
		}
	}

	bool exportRingBuffer = exportTask.effectIdx == -1;

	// 如果多个连续的效果都不能渲染，则合并为一个 CatmullRom
	uint32_t gapStartIdx = UINT_MAX;

	for (uint32_t effectIdx = 0; effectIdx < effectCount; ++effectIdx) {
		_EffectData& curEffectData = _effectDatas[effectIdx];

		if (!IsEffectReadyToDraw(effectDrawerStates[effectIdx])) {
			if (gapStartIdx == UINT_MAX) {
				gapStartIdx = effectIdx;
			}
			continue;
		}

		if (gapStartIdx != UINT_MAX) {
			assert(effectIdx > 0);
			const _EffectData& prevEffectData = _effectDatas[(uint64_t)effectIdx - 1];

			_DrawGap(
				computeContext,
				gapStartIdx,
				inputSrvOffset,
				prevEffectData.outputTexture.get(),
				prevEffectData.outputSize,
				_descriptorBaseOffset + effectIdx * 2 - 1
			);

			gapStartIdx = UINT_MAX;
		}

		if (effectIdx > 0) {
			computeContext.InsertTransitionBarrier(
				_effectDatas[(uint64_t)effectIdx - 1].outputTexture.get(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			);
		}

		bool writeToRingBuffer = effectIdx + 1 == effectCount && isLastEffectWriteToRingBuffer;
		bool isOutputAliasingSupported =
			bool(curEffectData.drawerInfo.flags & EffectDrawerFlags::SupportOutputAliasing);

		if (writeToRingBuffer) {
			computeContext.InsertTransitionBarrier(
				outputResource,
				D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
		} else if (!isOutputAliasingSupported) {
			// 如果效果没有 SupportOutputAliasing 标志则提前插入别名屏障。outputTexture
			// 在效果渲染期间始终有效。
			computeContext.InsertAliasingBarrier(nullptr, curEffectData.outputTexture.get());
		}

		bool exportCurEffect = (int)effectIdx == exportTask.effectIdx;
		bool exportInternalTexture = exportCurEffect && exportTask.passIdx >= 0 &&
			exportTask.passIdx < (int)curEffectData.debugInfo.passInfos.size() - 1;

		EffectDrawerExportData exportData = {
			.passIdx = exportInternalTexture ? exportTask.passIdx : -1,
			.outputIdx = exportTask.outputIdx
		};

		// 现在 curEffectData.outputTexture 可能处于非活跃状态，谨慎考虑不要执行状态转换。这就是我
		// 们让 outputTexture 长期处于 UNORDERED_ACCESS 状态的原因，这样有 SupportOutputAliasing
		// 标志的效果就不需要插入状态转换屏障。
		HRESULT hr = curEffectData.drawer->Draw(
			computeContext,
			effectIdx == 0 ? inputSrvOffset : _descriptorBaseOffset + (effectIdx - 1) * 2,
			writeToRingBuffer ? outputUavOffset : _descriptorBaseOffset + effectIdx * 2 + 1,
			!writeToRingBuffer && isOutputAliasingSupported ? curEffectData.outputTexture.get() : nullptr,
			exportData
		);
		if (FAILED(hr)) {
			Logger::Get().ComError(fmt::format("EffectDrawer[{}]::Draw 失败", effectIdx), hr);
			return hr;
		}

		if (effectIdx > 0) {
			computeContext.InsertTransitionBarrier(
				_effectDatas[(uint64_t)effectIdx - 1].outputTexture.get(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
		}

		if (exportInternalTexture) {
			_screenshotDrawer.ExportInternalTexture(std::move(exportData), frameFenceValue, effectIdx);
		}

		if (exportCurEffect && (exportTask.passIdx == -1 ||
			exportTask.passIdx == (int)curEffectData.debugInfo.passInfos.size() - 1))
		{
			// 导出当前效果的输出
			if (writeToRingBuffer) {
				exportRingBuffer = true;
			} else {
				computeContext.InsertTransitionBarrier(
					curEffectData.outputTexture.get(),
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
				);

				hr = _screenshotDrawer.Draw(computeContext, _descriptorBaseOffset + effectIdx * 2,
					curEffectData.outputSize, frameFenceValue);
				if (FAILED(hr)) {
					Logger::Get().ComError("ScreenshotDrawer::Draw 失败", hr);
					return hr;
				}

				computeContext.InsertTransitionBarrier(
					curEffectData.outputTexture.get(),
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS
				);
			}
		}
	}

	if (gapStartIdx == UINT_MAX && _effectDatas.back().outputSize != _outputSize) {
		gapStartIdx = effectCount;
	}

	if (gapStartIdx != UINT_MAX) {
		computeContext.InsertTransitionBarrier(
			outputResource,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);

		_DrawGap(computeContext, gapStartIdx, inputSrvOffset, outputResource, _outputSize, outputUavOffset);
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
	return S_OK;
}

HRESULT EffectsDrawer::OnColorInfoChanged(const ColorInfo& colorInfo) noexcept {
	bool wasSrgb = _colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange;
	bool isSrgb = colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange;

	_colorInfo = colorInfo;

	if (wasSrgb != isSrgb) {
		for (_EffectData& effectData : _effectDatas) {
			effectData.outputTexture = nullptr;
		}
	}

	_UpdateEffectBindings();

	_screenshotDrawer.OnColorInfoChanged(colorInfo);
	return S_OK;
}

void EffectsDrawer::OnProducerFrameCompleted(uint64_t fenceValue) noexcept {
	_screenshotDrawer.OnProducerFrameCompleted(fenceValue);
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

void EffectsDrawer::_UpdateEffectBindings() noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();

	_outputSize = _inputSize;

	uint32_t effectCount = (uint32_t)_effectDatas.size();
	for (uint32_t i = 0; i < effectCount; ++i) {
		_EffectData& effectData = _effectDatas[i];
		const EffectOption& effectOption = options.effects[i];

		// _outputSize 是前一个效果的输出尺寸，即当前效果的输入尺寸
		SizeU curOutputSize = CalcOutputSize(
			effectData.drawerInfo.scaleFactor, _outputSize, _rendererSize, effectOption);

		if (effectData.outputSize != curOutputSize) {
			effectData.outputSize = curOutputSize;
			effectData.outputTexture = nullptr;
		}

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

	// 如果直接渲染到环形缓冲那么最后一个效果就不需要输出纹理
	_EffectData& lastEffectData = _effectDatas.back();
	if (lastEffectData.outputSize == _outputSize) {
		lastEffectData.outputTexture = nullptr;
	}
}

HRESULT EffectsDrawer::_BuildMemPool(const SmallVectorImpl<EffectDrawerState>& effectDrawerStates) noexcept {
	auto& gpuResourceMaanger = _d3d12Context->GetGpuResourceManager();

	// 退休旧资源
	for (_EffectData& effectData : _effectDatas) {
		if (effectData.outputTexture) {
			gpuResourceMaanger.RetireResource(std::move(effectData.outputTexture), false);
		}
	}

	if (_defaultMemPoolAllocation) {
		gpuResourceMaanger.RetireResource(std::move(_defaultMemPoolAllocation), false);
	}
	if (_bufferMemPoolAllocation) {
		gpuResourceMaanger.RetireResource(std::move(_bufferMemPoolAllocation), false);
	}

	if (_descriptorBaseOffset != UINT_MAX) {
		gpuResourceMaanger.RetireDescriptors(
			_descriptorBaseOffset, _descriptorCount, false, false);
		_descriptorBaseOffset = UINT_MAX;
	}

	ID3D12Device5* device = _d3d12Context->GetDevice();
	D3D12MA::Allocator* allocator = gpuResourceMaanger.GetAllocator();
	auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();
	bool isResourceHeapTier2Supported = _d3d12Context->IsResourceHeapTier2Supported();

	const uint32_t effectCount = (uint32_t)_effectDatas.size();
	DXGI_FORMAT outputDxgiFormat = _colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange ?
		DXGI_FORMAT_R10G10B10A2_UNORM : DXGI_FORMAT_R16G16B16A16_FLOAT;
	// 如果不需要缩小，最后一个效果直接写入环形缓冲，不需要创建输出纹理
	bool isLastEffectWriteToRingBuffer = _effectDatas.back().outputSize == _outputSize;

	EffectsMemPoolBuilder memPoolBuilder(
		isResourceHeapTier2Supported, ScalingWindow::Get().Options().IsResourceAliasingDisabled());

	for (uint32_t i = 0; i < effectCount; ++i) {
		auto& effectData = _effectDatas[i];

		memPoolBuilder.BeginEffect();

		bool isEffectReadyToDraw = IsEffectReadyToDraw(effectDrawerStates[i]);
		// 两种情况下不需要创建输出纹理:
		// 1. 直接写入环形缓冲的最后一个效果
		// 2. 当前效果和下一个效果都不能渲染
		bool needOutputResource = i + 1 == effectCount ? !isLastEffectWriteToRingBuffer :
			(isEffectReadyToDraw || IsEffectReadyToDraw(effectDrawerStates[(uint64_t)i + 1]));
		bool isOutputAliasingSupported =
			bool(effectData.drawerInfo.flags & EffectDrawerFlags::SupportOutputAliasing);

		// 如果效果没有 SupportOutputAliasing 标志那么应确保输出纹理在效果渲染期间始终有效，
		// 因此先创建输出纹理，否则先创建中间纹理。
		if (isOutputAliasingSupported && isEffectReadyToDraw) {
			effectData.drawer->BuildMemPool(memPoolBuilder);
		}

		if (needOutputResource) {
			CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
				outputDxgiFormat,
				effectData.outputSize.width,
				effectData.outputSize.height,
				1, 1, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
			);
			memPoolBuilder.AllocResource(device->GetResourceAllocationInfo(0, 1, &resDesc), false, true);
		}

		if (!isOutputAliasingSupported && isEffectReadyToDraw) {
			effectData.drawer->BuildMemPool(memPoolBuilder);
		}
	}

	{
		D3D12MA::CALLOCATION_DESC allocDesc(
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12MA::ALLOCATION_FLAG_NONE,
			nullptr,
			D3D12MA_RECOMMENDED_HEAP_FLAGS | (isResourceHeapTier2Supported ?
				D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES : D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES)
		);

		D3D12_RESOURCE_ALLOCATION_INFO allocInfo = {
			.SizeInBytes = memPoolBuilder.GetCapacity(false),
			.Alignment = memPoolBuilder.GetAlignment(false)
		};

		if (allocInfo.SizeInBytes > 0) {
			// Allocator::AllocateMemory 要求 SizeInBytes 是 64KiB 的整数倍
			allocInfo.SizeInBytes = DirectXHelper::Align(allocInfo.SizeInBytes, 64ull << 10);

			HRESULT hr = allocator->AllocateMemory(&allocDesc, &allocInfo, _defaultMemPoolAllocation.put());
			if (FAILED(hr)) {
				Logger::Get().ComError("D3D12MA::Allocator::AllocateMemory 失败", hr);
				return hr;
			}
		} else {
			_defaultMemPoolAllocation = nullptr;
		}

		if (!isResourceHeapTier2Supported) {
			allocInfo.SizeInBytes = memPoolBuilder.GetCapacity(true);

			if (allocInfo.SizeInBytes > 0) {
				allocInfo.SizeInBytes = DirectXHelper::Align(allocInfo.SizeInBytes, 64ull << 10);
				allocDesc.ExtraHeapFlags = D3D12MA_RECOMMENDED_HEAP_FLAGS | D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
				allocInfo.Alignment = memPoolBuilder.GetAlignment(true);

				HRESULT hr = allocator->AllocateMemory(&allocDesc, &allocInfo, _bufferMemPoolAllocation.put());
				if (FAILED(hr)) {
					Logger::Get().ComError("D3D12MA::Allocator::AllocateMemory 失败", hr);
					return hr;
				}
			}
		}
	}

	SmallVector<SmallVector<uint64_t>, 0> resourceOffsets = memPoolBuilder.GetResourceOffsets();

	for (uint32_t i = 0; i < effectCount; ++i) {
		_EffectData& effectData = _effectDatas[i];
		const SmallVectorImpl<uint64_t>& effectResourceOffsets = resourceOffsets[i];

		bool isEffectReadyToDraw = IsEffectReadyToDraw(effectDrawerStates[i]);
		bool needOutputResource = i + 1 == effectCount ? !isLastEffectWriteToRingBuffer :
			(isEffectReadyToDraw || IsEffectReadyToDraw(effectDrawerStates[(uint64_t)i + 1]));
		// 如果效果有 SupportOutputAliasing 标志那么输出纹理是最后一个资源，否则是第一个资源
		bool isOutputAliasingSupported =
			bool(effectData.drawerInfo.flags & EffectDrawerFlags::SupportOutputAliasing);

		assert(!(needOutputResource && effectResourceOffsets.empty()));

		if (isEffectReadyToDraw) {
			std::span<const uint64_t> effectDrawerResourceOffsets;
			if (needOutputResource) {
				if (isOutputAliasingSupported) {
					effectDrawerResourceOffsets =
						std::span(effectResourceOffsets.begin(), effectResourceOffsets.end() - 1);
				} else {
					effectDrawerResourceOffsets =
						std::span(effectResourceOffsets.begin() + 1, effectResourceOffsets.end());
				}
			} else {
				effectDrawerResourceOffsets = effectResourceOffsets;
			}

			HRESULT hr = effectData.drawer->CreateResourcesInMemPool(
				_defaultMemPoolAllocation.get(),
				isResourceHeapTier2Supported ? _defaultMemPoolAllocation.get() : _bufferMemPoolAllocation.get(),
				effectDrawerResourceOffsets
			);
			if (hr != E_NOTIMPL && FAILED(hr)) {
				Logger::Get().ComError(fmt::format("EffectDrawer[{}]::CreateResourcesInMemPool 失败", i), hr);
				return hr;
			}
		}

		if (!needOutputResource) {
			continue;
		}

		CD3DX12_RESOURCE_DESC outputDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			outputDxgiFormat,
			effectData.outputSize.width,
			effectData.outputSize.height,
			1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		);

		HRESULT hr = allocator->CreateAliasingResource(
			_defaultMemPoolAllocation.get(),
			isOutputAliasingSupported ? effectResourceOffsets.back() : effectResourceOffsets[0],
			&outputDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&effectData.outputTexture)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("D3D12MA::Allocator::CreateAliasingResource 失败", hr);
			return hr;
		}
	}

	// 直接写入环形缓冲就不需要最后一个效果输出纹理的描述符。为了方便处理不排除当前不能渲染的效果
	_descriptorCount = effectCount * 2 - (isLastEffectWriteToRingBuffer ? 2 : 0);
	if (_descriptorCount == 0) {
		return S_OK;
	}

	HRESULT hr = descriptorHeap.Alloc(_descriptorCount, _descriptorBaseOffset);
	if (FAILED(hr)) {
		Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
		return hr;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE baseHandle(descriptorHeap.GetCpuHandle(_descriptorBaseOffset));
	uint32_t descriptorSize = descriptorHeap.GetDescriptorSize();

	CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc =
		CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(outputDxgiFormat, 1);
	CD3DX12_UNORDERED_ACCESS_VIEW_DESC uavDesc =
		CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2D(outputDxgiFormat);

	for (uint32_t i = 0; i < effectCount; ++i) {
		auto& effectData = _effectDatas[i];

		if (!effectData.outputTexture) {
			continue;
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(baseHandle, i * 2, descriptorSize);
		device->CreateShaderResourceView(effectData.outputTexture.get(), &srvDesc, cpuHandle);

		cpuHandle.Offset(descriptorSize);
		device->CreateUnorderedAccessView(effectData.outputTexture.get(), nullptr, &uavDesc, cpuHandle);
	}

	return S_OK;
}

void EffectsDrawer::_DrawGap(
	ComputeContext& computeContext,
	uint32_t startEffectIdx,
	uint32_t effectInputSrvOffset,
	ID3D12Resource* outputTexture,
	SizeU outputSize,
	uint32_t outputUavOffset
) noexcept {
	ID3D12Resource* inputTexture = nullptr;
	SizeU inputSize;
	uint32_t inputSrv;

	if (startEffectIdx == 0) {
		inputSize = _inputSize;
		inputSrv = effectInputSrvOffset;
	} else {
		uint32_t inputEffectIdx = startEffectIdx - 1;
		inputTexture = _effectDatas[inputEffectIdx].outputTexture.get();
		inputSize = _effectDatas[inputEffectIdx].outputSize;
		inputSrv = _descriptorBaseOffset + inputEffectIdx * 2;
	}

	if (inputTexture) {
		computeContext.InsertTransitionBarrier(
			inputTexture,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);
	}

	computeContext.InsertAliasingBarrier(nullptr, outputTexture);

	_catmullRomDrawer.Draw(
		computeContext,
		inputSize,
		outputSize,
		inputSrv,
		outputUavOffset
	);

	if (inputTexture) {
		computeContext.InsertTransitionBarrier(
			inputTexture,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);
	}
}

}
