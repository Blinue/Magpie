#include "pch.h"
#include "SwapChainPresenter.h"
#include "DebugInfo.h"
#include "DescriptorHeap.h"
#include "D3D12Context.h"
#include "Logger.h"
#include "Win32Helper.h"
#include <dcomp.h>
#include <dwmapi.h>

namespace Magpie {

SwapChainPresenter::~SwapChainPresenter() noexcept {
#ifdef _DEBUG
	if (_d3d12Context) {
		auto& rtvDescriptorHeap = _d3d12Context->GetDescriptorHeap(true);

		if (_rtvBaseOffset != std::numeric_limits<uint32_t>::max()) {
			rtvDescriptorHeap.Free(_rtvBaseOffset, _bufferCount);
		}

		if (_rawRtvBaseOffset != std::numeric_limits<uint32_t>::max()) {
			rtvDescriptorHeap.Free(_rawRtvBaseOffset, _bufferCount);
		}
	}
#endif
}

bool SwapChainPresenter::Initialize(
	D3D12Context& d3d12Context,
	HWND hwndAttach,
	SizeU size,
	const ColorInfo& colorInfo
) noexcept {
	_d3d12Context = &d3d12Context;
	_size = size;
	_isScRGB = colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;

	IDXGIFactory7* dxgiFactory = d3d12Context.GetDXGIFactory();

	// 检查撕裂支持
	{
		BOOL supportTearing = FALSE;
		HRESULT hr = dxgiFactory->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING, &supportTearing, sizeof(supportTearing));
		if (FAILED(hr)) {
			Logger::Get().ComWarn("IDXGIFactory5::CheckFeatureSupport 失败", hr);
		}

		_isTearingSupported = supportTearing;
	}

	_bufferCount = d3d12Context.GetMaxInFlightFrameCount() + 1;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
		.Width = size.width,
		.Height = size.height,
		// 默认色域正是我们想要的，无需额外设置
		.Format = _isScRGB ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM,
		.SampleDesc = {
			.Count = 1
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = _bufferCount,
#ifdef _DEBUG
		// 使边缘闪烁更容易观察到
		.Scaling = DXGI_SCALING_NONE,
#else
		// 使视觉变化尽可能小
		.Scaling = DXGI_SCALING_STRETCH,
#endif
		// 渲染每帧之前都会清空后缓冲区，因此无需 DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.AlphaMode = DXGI_ALPHA_MODE_IGNORE,
		// 支持时始终启用 DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
		.Flags = UINT((_isTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0)
		| DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
	};

	winrt::com_ptr<IDXGISwapChain1> dxgiSwapChain;
	HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(
		d3d12Context.GetCommandQueue(),
		hwndAttach,
		&swapChainDesc,
		nullptr,
		nullptr,
		dxgiSwapChain.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("IDXGIFactory2::CreateSwapChainForHwnd 失败", hr);
		return false;
	}

	_dxgiSwapChain = dxgiSwapChain.try_as<IDXGISwapChain4>();
	if (!_dxgiSwapChain) {
		Logger::Get().Error("检索 IDXGISwapChain4 失败");
		return false;
	}

	hr = _dxgiSwapChain->SetMaximumFrameLatency(_bufferCount - 1);
	if (FAILED(hr)) {
		Logger::Get().ComError("IDXGISwapChain2::SetMaximumFrameLatency 失败", hr);
		return false;
	}

	_frameLatencyWaitableObject.reset(_dxgiSwapChain->GetFrameLatencyWaitableObject());
	if (!_frameLatencyWaitableObject) {
		Logger::Get().Error("IDXGISwapChain2::GetFrameLatencyWaitableObject 失败");
		return false;
	}

	dxgiFactory->MakeWindowAssociation(hwndAttach, DXGI_MWA_NO_ALT_ENTER);

	_frameBuffers.resize(_bufferCount);

	{
		auto& rtvDescriptorHeap = d3d12Context.GetDescriptorHeap(true);

		hr = rtvDescriptorHeap.Alloc(_bufferCount, _rtvBaseOffset);
		if (FAILED(hr)) {
			Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
			return false;
		}

		// sRGB 下额外需要无伽马校正的 RTV
		if (!_isScRGB) {
			hr = rtvDescriptorHeap.Alloc(_bufferCount, _rawRtvBaseOffset);
			if (FAILED(hr)) {
				Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
				return hr;
			}
		}
	}
	
	hr = _CreateDisplayDependentResources();
	if (FAILED(hr)) {
		Logger::Get().ComError("_CreateDisplayDependentResources 失败", hr);
		return false;
	}

	return true;
}

void SwapChainPresenter::BeginFrame(
	ID3D12Resource** backBuffer,
	uint32_t& rtvOffset,
	uint32_t& rawRtvOffse
) noexcept {
	_frameLatencyWaitableObject.wait(1000);

	const uint32_t curBufferIndex = _dxgiSwapChain->GetCurrentBackBufferIndex();
	*backBuffer = _frameBuffers[curBufferIndex].get();

	rtvOffset = _rtvBaseOffset + curBufferIndex;
	if (!_isScRGB) {
		rawRtvOffse = _rawRtvBaseOffset + curBufferIndex;
	}
}

HRESULT SwapChainPresenter::EndFrame(bool waitForGpu) noexcept {
#ifdef MP_DEBUG_INFO
	{
		auto lk = DEBUG_INFO.lock.lock_exclusive();

		if (DEBUG_INFO.dtmFrameNumer != 0) {
			bool restartMeasure = false;

			if (DEBUG_INFO.dtmSwapChainRefreshCount == 0) {
				if (DEBUG_INFO.dtmFrameNumer == DEBUG_INFO.consumerFrameNumber) {
					// 追踪的帧将被呈现，记录当前交换链 VSync 计数
					DXGI_FRAME_STATISTICS statistics;
					HRESULT hr = _dxgiSwapChain->GetFrameStatistics(&statistics);
					if (SUCCEEDED(hr)) {
						DEBUG_INFO.dtmSwapChainRefreshCount = statistics.SyncRefreshCount;
					} else {
						restartMeasure = true;
					}
				} else if (DEBUG_INFO.dtmFrameNumer < DEBUG_INFO.consumerFrameNumber) {
					// 追踪的帧被错过，应重新测量
					restartMeasure = true;
				}
			} else {
				DXGI_FRAME_STATISTICS statistics;
				HRESULT hr = _dxgiSwapChain->GetFrameStatistics(&statistics);
				if (SUCCEEDED(hr)) {
					if (statistics.SyncRefreshCount != DEBUG_INFO.dtmSwapChainRefreshCount) {
						// 追踪的帧已被呈现
						LARGE_INTEGER frequency;
						QueryPerformanceFrequency(&frequency);

						DEBUG_INFO.dwmToMagpieLatency =
							int32_t((statistics.SyncQPCTime.QuadPart - DEBUG_INFO.dtmDwmQPC) * 1000000LL / frequency.QuadPart);
						restartMeasure = true;
					}
				} else {
					restartMeasure = true;
				}
			}

			if (restartMeasure) {
				DEBUG_INFO.dtmFrameNumer = 0;
				DEBUG_INFO.dtmSwapChainRefreshCount = 0;
			}
		}
	}
#endif

	const bool isRecreated = std::exchange(_isRecreated, false);
	if (isRecreated || waitForGpu) {
		// 下面两个调用用于减少调整窗口尺寸时的边缘闪烁。
		// 
		// 我们希望 DWM 绘制新的窗口框架时刚好合成新帧，但这不是我们能控制的，尤其是混合架构
		// 下需要在显卡间传输帧数据，无法预测 Present/Commit 后多久 DWM 能收到。我们只能尽
		// 可能为 DWM 合成新帧预留时间，这包括两个步骤：
		// 
		// 1. 首先等待渲染完成，确保新帧对 DWM 随时可用。
		// 2. 然后在新一轮合成开始时提交，这让 DWM 有更多时间合成新帧。
		// 
		// 目前看来除非像 UWP 一般有 DWM 协助，否则彻底摆脱闪烁是不可能的。
		// 
		// https://github.com/Blinue/Magpie/pull/1071#issuecomment-2718314731 讨论了 UWP
		// 调整尺寸的方法，测试表明可以彻底解决闪烁问题。不过它使用了很不稳定的私有接口，没有
		// 实用价值。

		// 等待渲染完成
		HRESULT hr = _d3d12Context->WaitForGpu();
		if (FAILED(hr)) {
			Logger::Get().ComError("D3D12Context::WaitForGPU", hr);
			return hr;
		}

		// 等待 DWM 开始合成新一帧
		Win32Helper::WaitForDwmComposition();
	}

	HRESULT hr = _dxgiSwapChain->Present(0, 0);
	if (FAILED(hr)) {
		Logger::Get().ComError("IDXGISwapChain::Present", hr);
		return hr;
	}

#ifdef MP_DEBUG_INFO
	{
		auto lk = DEBUG_INFO.lock.lock_exclusive();

		if (DEBUG_INFO.ctpFrameNumer != 0 && DEBUG_INFO.ctpFrameNumer <= DEBUG_INFO.consumerFrameNumber) {
			// 如果 ctpFrameNumer < consumerFrameNumber 那么追踪的帧被错过，应重新测量
			if (DEBUG_INFO.ctpFrameNumer == DEBUG_INFO.consumerFrameNumber) {
				LARGE_INTEGER counter;
				QueryPerformanceCounter(&counter);

				LARGE_INTEGER frequency;
				QueryPerformanceFrequency(&frequency);

				DEBUG_INFO.captureToPresentLatency =
					uint32_t((counter.QuadPart - DEBUG_INFO.ctpCaptureQPC) * 1000000LL / frequency.QuadPart);
			}

			DEBUG_INFO.ctpCapturedFrame = 0;
			DEBUG_INFO.ctpFrameNumer = 0;
		}
	}
#endif

	return S_OK;
}

HRESULT SwapChainPresenter::OnResizingChanged(bool value) noexcept {
	// 开始调整尺寸时推迟到尺寸变化再重建交换链
	_isResizing = value;

	if (!value) {
		// 恢复后备缓冲数量
		const uint32_t oldBufferCount = _bufferCount;
		_bufferCount = _d3d12Context->GetMaxInFlightFrameCount() + 1;

		if (_bufferCount != oldBufferCount) {
			// 调用此方法前没等待 GPU
			HRESULT hr = _d3d12Context->WaitForGpu();
			if (FAILED(hr)) {
				Logger::Get().ComError("D3D12Context::WaitForGPU", hr);
				return hr;
			}

			hr = _RecreateBuffers();
			if (FAILED(hr)) {
				Logger::Get().ComError("_RecreateBuffers 失败", hr);
				return hr;
			}
		}
	}

	return S_OK;
}

HRESULT SwapChainPresenter::OnResized(SizeU size) noexcept {
	assert(size.width > 0 && size.height > 0 && size != _size);

	_size = size;
	// 调整大小期间只用两个后备缓冲以提高流畅度并减少边缘闪烁
	_bufferCount = _isResizing ? 2 : _d3d12Context->GetMaxInFlightFrameCount() + 1;

	HRESULT hr = _RecreateBuffers();
	if (FAILED(hr)) {
		Logger::Get().ComError("_RecreateBuffers 失败", hr);
	}

	return hr;
}

HRESULT SwapChainPresenter::OnColorInfoChanged(const ColorInfo& colorInfo) noexcept {
	const bool wasScRGB = _isScRGB;
	_isScRGB = colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;

	if (_isScRGB == wasScRGB) {
		return S_OK;
	}

	auto& rtvDescriptorHeap = _d3d12Context->GetDescriptorHeap(true);
	if (_isScRGB) {
		if (_rawRtvBaseOffset != std::numeric_limits<uint32_t>::max()) {
			rtvDescriptorHeap.Free(_rawRtvBaseOffset, _bufferCount);
			_rawRtvBaseOffset = std::numeric_limits<uint32_t>::max();
		}
	} else {
		if (_rawRtvBaseOffset == std::numeric_limits<uint32_t>::max()) {
			HRESULT hr = rtvDescriptorHeap.Alloc(_bufferCount, _rawRtvBaseOffset);
			if (FAILED(hr)) {
				Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
				return hr;
			}
		}
	}

	HRESULT hr = _RecreateBuffers();
	if (FAILED(hr)) {
		Logger::Get().ComError("_RecreateBuffers 失败", hr);
		return hr;
	}

	hr = _dxgiSwapChain->SetColorSpace1(
		_isScRGB ? DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
	if (FAILED(hr)) {
		Logger::Get().ComError("IDXGISwapChain4::SetColorSpace1 失败", hr);
	}

	return hr;
}

HRESULT SwapChainPresenter::_RecreateBuffers() noexcept {
	std::fill(_frameBuffers.begin(), _frameBuffers.end(), nullptr);

	// 不要更改最大帧延迟，一来调整大小期间不会有帧排队，二来交换链不大支持中途改变
	// 最大帧延迟，需要额外等待 FrameLatencyWaitableObject 来修正内部状态。
	HRESULT hr = _dxgiSwapChain->ResizeBuffers(
		_bufferCount, _size.width, _size.height,
		_isScRGB ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM,
		UINT((_isTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0)
			| DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("IDXGISwapChain::ResizeBuffers", hr);
		return hr;
	}

	_isRecreated = true;

	hr = _CreateDisplayDependentResources();
	if (FAILED(hr)) {
		Logger::Get().ComError("_CreateDisplayDependentResources 失败", hr);
	}

	return hr;
}

HRESULT SwapChainPresenter::_CreateDisplayDependentResources() noexcept {
	ID3D12Device5* device = _d3d12Context->GetDevice();
	auto& rtvDescriptorHeap = _d3d12Context->GetDescriptorHeap(true);

	uint32_t descriptorSize = rtvDescriptorHeap.GetDescriptorSize();
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap.GetCpuHandle(_rtvBaseOffset));
	CD3DX12_CPU_DESCRIPTOR_HANDLE rawRtvHandle{};
	if (!_isScRGB) {
		rawRtvHandle = rtvDescriptorHeap.GetCpuHandle(_rawRtvBaseOffset);
	}
	
	for (uint32_t i = 0; i < _bufferCount; ++i) {
		HRESULT hr = _dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&_frameBuffers[i]));
		if (FAILED(hr)) {
			Logger::Get().ComError("IDXGISwapChain::GetBuffer 失败", hr);
			return hr;
		}

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {
			.Format = _isScRGB ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D
		};
		device->CreateRenderTargetView(_frameBuffers[i].get(), &rtvDesc, rtvHandle);
		rtvHandle.Offset(descriptorSize);

		if (!_isScRGB) {
			rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			device->CreateRenderTargetView(_frameBuffers[i].get(), &rtvDesc, rawRtvHandle);
			rawRtvHandle.Offset(descriptorSize);
		}
	}

	return S_OK;
}

}
