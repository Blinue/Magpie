#include "pch.h"
#include "SwapChainPresenter.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "DeviceResources.h"

namespace Magpie {

bool SwapChainPresenter::_Initialize(HWND hwndAttach) noexcept {
	// 为了降低延迟，两个垂直同步之间允许渲染 BUFFER_COUNT - 1 帧
	// 如果这个值太小，用户移动光标可能造成画面卡顿
	static constexpr uint32_t BUFFER_COUNT = 4;

	const RECT& rendererRect = ScalingWindow::Get().RendererRect();
	DXGI_SWAP_CHAIN_DESC1 sd{
		.Width = UINT(rendererRect.right - rendererRect.left),
		.Height = UINT(rendererRect.bottom - rendererRect.top),
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.SampleDesc = {
			.Count = 1
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = BUFFER_COUNT,
		.Scaling = DXGI_SCALING_NONE,
		// 渲染每帧之前都会清空后缓冲区，因此无需 DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.AlphaMode = DXGI_ALPHA_MODE_IGNORE,
		// 只要显卡支持始终启用 DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING 以支持可变刷新率
		.Flags = UINT((_deviceResources->IsTearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0)
		| DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
	};

	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();
	winrt::com_ptr<IDXGISwapChain1> dxgiSwapChain = nullptr;
	HRESULT hr = _deviceResources->GetDXGIFactory()->CreateSwapChainForHwnd(
		d3dDevice,
		hwndAttach,
		&sd,
		nullptr,
		nullptr,
		dxgiSwapChain.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("创建交换链失败", hr);
		return false;
	}

	_swapChain = dxgiSwapChain.try_as<IDXGISwapChain4>();
	if (!_swapChain) {
		Logger::Get().Error("获取 IDXGISwapChain2 失败");
		return false;
	}

	// 允许提前渲染 BUFFER_COUNT - 1 帧
	_swapChain->SetMaximumFrameLatency(BUFFER_COUNT - 1);

	_frameLatencyWaitableObject.reset(_swapChain->GetFrameLatencyWaitableObject());
	if (!_frameLatencyWaitableObject) {
		Logger::Get().Error("GetFrameLatencyWaitableObject 失败");
		return false;
	}

	hr = _deviceResources->GetDXGIFactory()->MakeWindowAssociation(
		hwndAttach, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hr)) {
		Logger::Get().ComError("MakeWindowAssociation 失败", hr);
	}

	hr = _swapChain->GetBuffer(0, IID_PPV_ARGS(_backBuffer.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("获取后缓冲区失败", hr);
		return false;
	}

	hr = d3dDevice->CreateRenderTargetView(_backBuffer.get(), nullptr, _backBufferRtv.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateRenderTargetView 失败", hr);
		return false;
	}

	return true;
}

winrt::com_ptr<ID3D11RenderTargetView> SwapChainPresenter::BeginFrame(POINT& updateOffset) noexcept {
	updateOffset = {};

	if(!_isframeLatencyWaited) {
		_frameLatencyWaitableObject.wait(1000);
		_isframeLatencyWaited = true;
	}

	return _backBufferRtv;
}

void SwapChainPresenter::EndFrame() noexcept {
	if (_isResized) {
		_isResized = false;
		_WaitForDwmAfterResize();
	}

	// 两个垂直同步之间允许渲染数帧，SyncInterval = 0 只呈现最新的一帧，旧帧被丢弃
	_swapChain->Present(0, 0);
	_isframeLatencyWaited = false;

	// 丢弃渲染目标的内容
	_deviceResources->GetD3DDC()->DiscardView(_backBufferRtv.get());
}

bool SwapChainPresenter::Resize() noexcept {
	if (!_isframeLatencyWaited) {
		_frameLatencyWaitableObject.wait(1000);
		_isframeLatencyWaited = true;
	}

	_backBuffer = nullptr;
	_backBufferRtv = nullptr;

	const RECT& swapChainRect = ScalingWindow::Get().RendererRect();
	const SIZE swapChainSize = Win32Helper::GetSizeOfRect(swapChainRect);
	HRESULT hr = _swapChain->ResizeBuffers(
		0,
		(UINT)swapChainSize.cx,
		(UINT)swapChainSize.cy,
		DXGI_FORMAT_UNKNOWN,
		UINT((_deviceResources->IsTearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0)
		| DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("ResizeBuffers 失败", hr);
		return false;
	}

	hr = _swapChain->GetBuffer(0, IID_PPV_ARGS(_backBuffer.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("获取后缓冲区失败", hr);
		return false;
	}

	hr = _deviceResources->GetD3DDevice()->CreateRenderTargetView(
		_backBuffer.get(), nullptr, _backBufferRtv.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateRenderTargetView 失败", hr);
		return false;
	}

	_isResized = true;
	return true;
}

}
