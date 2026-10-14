#include "pch.h"
#include "EffectsDrawer.h"
#include "CatmullRomDrawer.h"
#include "CommandContext.h"
#include "D3D12Context.h"
#include "DescriptorHeap.h"
#include "DirectXHelper.h"
#include "EffectInfo.h"
#include "LocalizationService.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "ScreenshotFilenameTemplateHelper.h"
#include "ShaderEffectDrawer.h"
#include "shaders/ScreenshotCS.h"
#include "shaders/ScreenshotCS_HDR.h"
#include "shaders/ScreenshotCS_HDR_SM5.h"
#include "shaders/ScreenshotCS_SM5.h"
#include "shaders/ScreenshotCS_WCG.h"
#include "shaders/ScreenshotCS_WCG_SM5.h"
#include "TextureHelper.h"
#include "Win32Helper.h"

namespace Magpie {

EffectsDrawer::~EffectsDrawer() noexcept {
	// 等待截图完成
	MSG msg;
	while (_pendingScreenshotCount != 0) {
		// 必须处理消息队列，否则协程无法执行
		WaitMessage();

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			DispatchMessage(&msg);
		}
	}

#ifdef _DEBUG
	if (_descriptorBaseOffset != std::numeric_limits<uint32_t>::max()) {
		_d3d12Context->GetDescriptorHeap().Free(_descriptorBaseOffset, _CalcDescriptorCount());
	}

	for (const _ScreenshotData& data : _screenshotDatas) {
		if (data.descriptorBaseOffset != std::numeric_limits<uint32_t>::max()) {
			_d3d12Context->GetDescriptorHeap().Free(
				data.descriptorBaseOffset, data.isExportingRingBuffer ? 2 : 1);
		}
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
	ID3D12Resource* outputResource,
	uint32_t inputSrvOffset,
	uint32_t outputUavOffset,
	uint64_t completedFenceValue,
	uint64_t frameFenceValue
) noexcept {
	const std::pair<int, int> screenshotTask =
		_screenshotTask.exchange(std::make_pair(-3, 0), std::memory_order_relaxed);

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
				_descriptorBaseOffset + effectIdx * 2 - 1
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

		if (writeToRingBuffer) {
			computeContext.InsertTransitionBarrier(
				outputResource,
				D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
		} else {
			computeContext.InsertTransitionBarrier(
				_effectDatas[effectIdx].outputTexture.get(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
		}

		hr = _effectDatas[effectIdx].drawer->Draw(
			computeContext,
			effectIdx == 0 ? inputSrvOffset : _descriptorBaseOffset + (effectIdx - 1) * 2,
			writeToRingBuffer ? outputUavOffset : _descriptorBaseOffset + effectIdx * 2 + 1,
			(int)effectIdx == screenshotTask.first ? screenshotTask.second : -2
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

		computeContext.InsertTransitionBarrier(
			outputResource,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);

		_catmullRomDrawer.Draw(
			computeContext, inputSize, _outputSize, inputSrv, outputUavOffset);
	} else if (_effectDatas.back().outputSize != _outputSize) {
		computeContext.InsertTransitionBarrier(
			_effectDatas.back().outputTexture.get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);

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

	if (screenshotTask.first == -1) {
		// 导出最终输出画面
		ID3D12Device5* device = _d3d12Context->GetDevice();
		
		// R16G16B16A16_FLOAT 每像素 8 个字节
		uint32_t readbackRowSize = _outputSize.width * 8u;
		uint32_t readbackRowPitch = DirectXHelper::Align(readbackRowSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		uint32_t requiredBufferSize = readbackRowPitch * _outputSize.height;

		// 寻找空闲且足够大的回读缓冲区
		uint32_t reuseDataIdx = std::numeric_limits<uint32_t>::max();
		for (uint32_t i = 0, end = (uint32_t)_screenshotDatas.size(); i < end; ++i) {
			const _ScreenshotData& curData = _screenshotDatas[i];

			if (!curData.isExported || curData.fenceValue > completedFenceValue) {
				continue;
			}

			if (curData.bufferSize >= requiredBufferSize) {
				reuseDataIdx = i;
				break;
			}
		}

		_ScreenshotData* curData;

		if (reuseDataIdx == std::numeric_limits<uint32_t>::max()) {
			_ScreenshotData newData = {
				.bufferSize = requiredBufferSize
			};

			CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_READBACK);

			D3D12_HEAP_FLAGS heapFlags = _d3d12Context->IsHeapFlagCreateNotZeroedSupported() ?
				D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

			CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(newData.bufferSize);

			HRESULT hr = device->CreateCommittedResource(
				&heapProps,
				heapFlags,
				&bufferDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&newData.buffer)
			);
			if (FAILED(hr)) {
				Logger::Get().ComError("CreateCommittedResource 失败", hr);
				return hr;
			}

			hr = newData.buffer->Map(0, nullptr, &newData.bufferData);
			if (FAILED(hr)) {
				Logger::Get().ComError("ID3D12Resource::Map 失败", hr);
				return hr;
			}

			curData = &_screenshotDatas.emplace_back(std::move(newData));
		} else {
			curData = &_screenshotDatas[reuseDataIdx];

			curData->isExported = false;
			curData->tempTexture = nullptr;
			if (curData->descriptorBaseOffset != std::numeric_limits<uint32_t>::max()) {
				auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();
				descriptorHeap.Free(curData->descriptorBaseOffset, curData->isExportingRingBuffer ? 2 : 1);
			}
		}

		curData->fenceValue = frameFenceValue;
		curData->textureSize = _outputSize;
		curData->rowPitch = readbackRowPitch;
		curData->acKind = _colorInfo.kind;
		curData->isExportingRingBuffer = true;

		// 把要导出的纹理转换为用于导出截图的格式。
		// SDR: 需做伽马校正
		// WCG: 需转换为 BT.2020
		// HDR: 需转换为 BT.2020/PQ
		computeContext.InsertTransitionBarrier(
			outputResource,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);

		// 纹理格式始终是 R16G16B16A16_UNORM
		CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R16G16B16A16_UNORM,
			_outputSize.width, _outputSize.height, 1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		);

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

		D3D12_HEAP_FLAGS heapFlags = _d3d12Context->IsHeapFlagCreateNotZeroedSupported() ?
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

		HRESULT hr = device->CreateCommittedResource(
			&heapProps, heapFlags, &texDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr, IID_PPV_ARGS(&curData->tempTexture));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommittedResource 失败", hr);
			return hr;
		}

		auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();
		hr = descriptorHeap.Alloc(2, curData->descriptorBaseOffset);
		if (FAILED(hr)) {
			Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
			return hr;
		}

		CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc =
			CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1);
		CD3DX12_UNORDERED_ACCESS_VIEW_DESC uavDesc =
			CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2D(texDesc.Format);

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptorHeap.GetCpuHandle(curData->descriptorBaseOffset));
		device->CreateShaderResourceView(outputResource, &srvDesc, cpuHandle);
		cpuHandle.Offset(descriptorHeap.GetDescriptorSize());
		device->CreateUnorderedAccessView(curData->tempTexture.get(), nullptr, &uavDesc, cpuHandle);

		if (!_screenshotRootSignature) {
			hr = _InitializeScreenshotRootSignature();
			if (FAILED(hr)) {
				Logger::Get().ComError("_InitializeScreenshotRootSignature 失败", hr);
				return hr;
			}
		}
		computeContext.SetRootSignature(_screenshotRootSignature.get());

		if (!_screenshotPSO) {
			D3D12_SHADER_BYTECODE psByteCode;
			bool isSM6Supported = _d3d12Context->GetShaderModel() >= D3D_SHADER_MODEL_6_0;
			if (curData->acKind == winrt::AdvancedColorKind::StandardDynamicRange) {
				psByteCode = DirectXHelper::SelectShader(
					isSM6Supported, ScreenshotCS, ScreenshotCS_SM5);
			} else if (curData->acKind == winrt::AdvancedColorKind::WideColorGamut) {
				psByteCode = DirectXHelper::SelectShader(
					isSM6Supported, ScreenshotCS_WCG, ScreenshotCS_WCG_SM5);
			} else {
				psByteCode = DirectXHelper::SelectShader(
					isSM6Supported, ScreenshotCS_HDR, ScreenshotCS_HDR_SM5);
			}

			D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {
				.pRootSignature = _screenshotRootSignature.get(),
				.CS = psByteCode
			};
			hr = _d3d12Context->GetDevice()->CreateComputePipelineState(
				&psoDesc, IID_PPV_ARGS(&_screenshotPSO));
			if (FAILED(hr)) {
				Logger::Get().ComError("CreateComputePipelineState 失败", hr);
				return hr;
			}
		}

		computeContext.SetPipelineState(_screenshotPSO.get());

		computeContext.SetRootDescriptorTable(0, curData->descriptorBaseOffset);
		computeContext.SetRootDescriptorTable(1, curData->descriptorBaseOffset + 1);

		constexpr uint32_t BLOCK_SIZE = 16;
		computeContext.Dispatch(
			(_outputSize.width + BLOCK_SIZE - 1) / BLOCK_SIZE,
			(_outputSize.height + BLOCK_SIZE - 1) / BLOCK_SIZE
		);

		computeContext.InsertTransitionBarrier(
			curData->tempTexture.get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COPY_SOURCE
		);

		computeContext.CopyTextureRegion(
			CD3DX12_TEXTURE_COPY_LOCATION(
				curData->buffer.get(),
				D3D12_PLACED_SUBRESOURCE_FOOTPRINT{
					.Footprint = {
						.Format = DXGI_FORMAT_R16G16B16A16_UNORM,
						.Width = curData->textureSize.width,
						.Height = curData->textureSize.height,
						.Depth = 1,
						.RowPitch = curData->rowPitch
					}
				}
			),
			0,
			0,
			CD3DX12_TEXTURE_COPY_LOCATION(curData->tempTexture.get())
		);

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

	// 释放空闲的回读缓冲区
	for (int i = (int)_screenshotDatas.size() - 1; i >= 0; --i) {
		_ScreenshotData& curBuffer = _screenshotDatas[i];

		if (curBuffer.isExported) {
			assert(curBuffer.fenceValue <= completedFenceValue);

			if (curBuffer.descriptorBaseOffset != std::numeric_limits<uint32_t>::max()) {
				_d3d12Context->GetDescriptorHeap().Free(
					curBuffer.descriptorBaseOffset, curBuffer.isExportingRingBuffer ? 2 : 1);
			}

			_screenshotDatas.erase(_screenshotDatas.begin() + i);
		}
	}

	return S_OK;
}

void EffectsDrawer::TaskScreenshotAsync(int effectIdx, int passIdx) noexcept {
	assert(effectIdx >= -2 && passIdx >= -1);

	// 只保存一个截图任务
	_screenshotTask.store(std::make_pair(effectIdx, passIdx), std::memory_order_relaxed);

	// 立刻渲染导出截图
	ScalingWindow::Get().RequestNewFrame(true);
}

void EffectsDrawer::OnResized(SizeU rendererSize, SizeU& outputSize) noexcept {
	_rendererSize = rendererSize;
	_UpdateEffectBindings();
	outputSize = _outputSize;
}

void EffectsDrawer::OnColorInfoChanged(const ColorInfo& colorInfo) noexcept {
	const winrt::AdvancedColorKind oldKind = _colorInfo.kind;
	_colorInfo = colorInfo;

	_UpdateEffectBindings();

	if (colorInfo.kind != oldKind) {
		_screenshotPSO = nullptr;
	}
}

static void AppendSuffixToScreenshotFilename(std::wstring& screenshotFileName) noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();

	uint32_t suffixNum = 1;

	WIN32_FIND_DATA findData{};
	wil::unique_hfind hFind(FindFirstFileEx(
		StrHelper::Concat(options.screenshotsDir.native(), L"\\", screenshotFileName, L"*").c_str(),
		FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
	if (hFind) {
		do {
			std::wstring_view fileName(findData.cFileName);

			// 不考虑扩展名
			if (size_t dotPos = fileName.find_last_of(L'.'); dotPos != std::wstring_view::npos) {
				fileName.remove_suffix(fileName.size() - dotPos);
			}

			// 文件名重复时格式为 "{baseFileName} ({suffixNum})"

			// 原始文件名视为 1
			if (fileName.size() == screenshotFileName.size()) {
				assert(fileName == screenshotFileName);
				suffixNum = std::max(suffixNum, 2u);
				continue;
			}

			// 后缀至少 4 个字符
			if (fileName.size() < screenshotFileName.size() + 4) {
				continue;
			}

			fileName.remove_prefix(screenshotFileName.size());

			if (!fileName.starts_with(L" (") || !fileName.ends_with(L')')) {
				continue;
			}

			uint32_t curSuffixNum;
			std::string curSuffixNumStr = StrHelper::UTF16ToUTF8(
				std::wstring_view(fileName.data() + 2, fileName.size() - 3));
			const char* end = curSuffixNumStr.data() + curSuffixNumStr.size();

			std::from_chars_result result = std::from_chars(
				curSuffixNumStr.data(), end, curSuffixNum);
			if (result.ec != std::errc{} || result.ptr != end || curSuffixNum < 2) {
				continue;
			}

			suffixNum = std::max(suffixNum, curSuffixNum + 1);
		} while (FindNextFile(hFind.get(), &findData));
	}

	if (suffixNum > 1) {
		screenshotFileName += L" (";
		screenshotFileName += StrHelper::ToWString(suffixNum);
		screenshotFileName += L')';
	}
}

enum class TakeScreenshotResult {
	Success,
	InvalidDirectory,
	InvalidFilenameTemplate,
	InternalError
};

static TakeScreenshotResult ExportScreenshot(
	HWND hwndSrc,
	std::wstring_view imgFormat,
	SizeU imgSize,
	EffectTextureFormat textureFormat,
	const std::span<uint8_t>& pixelData,
	uint32_t rowPitch,
	winrt::AdvancedColorKind acKind,
	std::wstring& screenshotFileName
) noexcept {
	// 确保截图保存目录存在
	const ScalingOptions& options = ScalingWindow::Get().Options();

	if (options.screenshotsDir.empty()) {
		return TakeScreenshotResult::InvalidDirectory;
	}

	if (!Win32Helper::CreateDir(options.screenshotsDir.c_str(), true)) {
		Logger::Get().Error("CreateDir 失败");
		return TakeScreenshotResult::InvalidDirectory;
	}

	// 解析文件名模板
	{
		std::string fileNameUtf8;
		if (!ScreenshotFilenameTemplateHelper::Apply(
			options.screenshotFilenameTemplate, hwndSrc, fileNameUtf8)) {
			Logger::Get().Error("ScreenshotFilenameTemplateHelper::Apply 失败");
			return TakeScreenshotResult::InvalidFilenameTemplate;
		}

		screenshotFileName = StrHelper::UTF8ToUTF16(fileNameUtf8);
	}

	// 加锁以保证后缀的唯一性
	static wil::srwlock lock;
	{
		auto lk = lock.lock_exclusive();

		AppendSuffixToScreenshotFilename(screenshotFileName);

		screenshotFileName += L'.';
		screenshotFileName += imgFormat;

		std::filesystem::path screenshotPath = options.screenshotsDir / screenshotFileName;

		HRESULT hr = TextureHelper::Save(screenshotPath.c_str(), imgSize,
			textureFormat, pixelData, rowPitch, acKind);
		if (FAILED(hr)) {
			Logger::Get().ComError("TextureHelper::SaveTexture 失败", hr);
			return TakeScreenshotResult::InternalError;
		}
	}

	return TakeScreenshotResult::Success;
}

void EffectsDrawer::OnProducerFrameCompleted(uint64_t fenceValue) noexcept {
	for (_ScreenshotData& data : _screenshotDatas) {
		if (data.fenceValue > fenceValue) {
			return;
		}

		data.isExported = true;

		[&data, this](winrt::DispatcherQueue dispatcher) -> winrt::fire_and_forget {
			++_pendingScreenshotCount;

			// 确保即使协程出了问题也能恢复 _pendingScreenshotCount
			auto se = wil::scope_exit([&] {
				--_pendingScreenshotCount;
			});

			TakeScreenshotResult result;
			std::wstring screenshotFileName;
			{
				std::vector<uint8_t> pixelData(data.bufferSize);
				std::memcpy(pixelData.data(), data.bufferData, data.bufferSize);

				HWND hwndSrc = ScalingWindow::Get().SrcHandle();
				SizeU imgSize = data.textureSize;
				uint32_t rowPitch = data.rowPitch;
				winrt::AdvancedColorKind acKind = data.acKind;

				co_await winrt::resume_background();

				result = ExportScreenshot(hwndSrc, L"png", imgSize,
					EffectTextureFormat::R16G16B16A16_UNORM, pixelData,
					rowPitch, acKind, screenshotFileName);
			}
			
			co_await dispatcher;

			LocalizationService& ls = LocalizationService::Get();

			if (result == TakeScreenshotResult::Success) {
				winrt::hstring successMsg = ls.GetLocalizedString(L"Message_ScreenshotSaved");
				ScalingWindow::Get().ShowToast(
					fmt::format(fmt::runtime(std::wstring_view(successMsg)), screenshotFileName));
			} else {
				Logger::Get().Error("_TakeScreenshotImpl 失败");

				const wchar_t* errorMsgs[] = {
					L"Message_ScreenshotFailed_InvalidDirectory",
					L"Message_ScreenshotFailed_InvalidFilenameTemplate",
					L"Message_ScreenshotFailed_InternalError"
				};
				ScalingWindow::Get().ShowToast(
					ls.GetLocalizedString(L"Message_ScreenshotFailed_Title"),
					ls.GetLocalizedString(errorMsgs[(size_t)result - 1])
				);
			}
		}(_dispatcher);
	}
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

HRESULT EffectsDrawer::_InitializeScreenshotRootSignature() noexcept {
	winrt::com_ptr<ID3DBlob> signature;

	CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
	CD3DX12_DESCRIPTOR_RANGE1 uavRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

	D3D12_ROOT_PARAMETER1 rootParams[] = {
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			.DescriptorTable = {
				.NumDescriptorRanges = 1,
				.pDescriptorRanges = &srvRange
			}
		},
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			.DescriptorTable = {
				.NumDescriptorRanges = 1,
				.pDescriptorRanges = &uavRange
			}
		}
	};

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
		(UINT)std::size(rootParams), rootParams, 0, nullptr);

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

	hr = _d3d12Context->GetDevice()->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&_screenshotRootSignature)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateRootSignature 失败", hr);
		return hr;
	}

	return S_OK;
}

}
