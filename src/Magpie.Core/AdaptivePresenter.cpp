#include "pch.h"
#include "AdaptivePresenter.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "DeviceResources.h"
#include "Win32Helper.h"

namespace Magpie {

bool AdaptivePresenter::_Initialize(HWND hwndAttach) noexcept {
	if (ScalingWindow::Get().Options().IsDirectFlipDisabled()) {
		// 禁用 DirectFlip 时始终使用 DirectComposition 呈现
		if (!_ResizeDCompVisual(hwndAttach)) {
			Logger::Get().Error("_ResizeDCompVisual 失败");
			return false;
		}

		return true;
	}

	const uint32_t bufferCount = _CalcBufferCount();

	const SIZE rendererSize = Win32Helper::GetSizeOfRect(ScalingWindow::Get().RendererRect());
	DXGI_SWAP_CHAIN_DESC1 sd{
		.Width = (UINT)rendererSize.cx,
		.Height = (UINT)rendererSize.cy,
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.SampleDesc = {
			.Count = 1
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = bufferCount,
#ifdef _DEBUG
		// 我们应确保两种渲染方式可以无缝切换，DXGI_SCALING_NONE 使错误更容易观察到
		.Scaling = DXGI_SCALING_NONE,
#else
		// 如果两种渲染方式无法无缝切换，DXGI_SCALING_STRETCH 使视觉变化尽可能小
		.Scaling = DXGI_SCALING_STRETCH,
#endif
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

	_dxgiSwapChain = dxgiSwapChain.try_as<IDXGISwapChain4>();
	if (!_dxgiSwapChain) {
		Logger::Get().Error("获取 IDXGISwapChain2 失败");
		return false;
	}

	// 为了降低延迟，两个垂直同步之间允许渲染 bufferCount - 1 帧
	_dxgiSwapChain->SetMaximumFrameLatency(bufferCount - 1);

	_frameLatencyWaitableObject.reset(_dxgiSwapChain->GetFrameLatencyWaitableObject());
	if (!_frameLatencyWaitableObject) {
		Logger::Get().Error("GetFrameLatencyWaitableObject 失败");
		return false;
	}

	hr = _deviceResources->GetDXGIFactory()->MakeWindowAssociation(
		hwndAttach, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hr)) {
		Logger::Get().ComError("MakeWindowAssociation 失败", hr);
	}

	hr = _dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(_backBuffer.put()));
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

bool AdaptivePresenter::BeginFrame(
	winrt::com_ptr<ID3D11Texture2D>& frameTex,
	winrt::com_ptr<ID3D11RenderTargetView>& frameRtv,
	POINT& drawOffset
) noexcept {
	if (_isDCompPresenting) {
		HRESULT hr = _dcompSurface->BeginDraw(nullptr, IID_PPV_ARGS(&frameTex), &drawOffset);
		if (FAILED(hr)) {
			Logger::Get().ComError("BeginDraw 失败", hr);
			return false;
		}

		hr = _deviceResources->GetD3DDevice()->CreateRenderTargetView(
			frameTex.get(), nullptr, frameRtv.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateRenderTargetView 失败", hr);
			return false;
		}
	} else {
		drawOffset = {};

		if (!_isframeLatencyWaited) {
			_frameLatencyWaitableObject.wait(1000);
			_isframeLatencyWaited = true;
		}

		frameTex = _backBuffer;
		frameRtv = _backBufferRtv;
	}
	
	return true;
}

void AdaptivePresenter::EndFrame(bool waitForRenderComplete) noexcept {
	if (_isDCompPresenting) {
		_dcompSurface->EndDraw();
	}

	if (waitForRenderComplete || _isResized) {
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
		_WaitForRenderComplete();

		// 等待 DWM 开始合成新一帧
		_WaitForDwmComposition();
	}

	if (_isDCompPresenting) {
		_dcompDevice->Commit();
	} else {
		// 两个垂直同步之间允许渲染数帧，SyncInterval = 0 只呈现最新的一帧，旧帧被丢弃
		_dxgiSwapChain->Present(0, 0);
		_isframeLatencyWaited = false;

		// 丢弃渲染目标的内容
		_deviceResources->GetD3DDC()->DiscardView(_backBufferRtv.get());

		if (_isSwitchingToSwapChain) {
			_isSwitchingToSwapChain = false;

			// 等待交换链呈现新帧
			_WaitForRenderComplete();
			_WaitForDwmComposition();

			// 清除 DirectCompostion 内容
			_dcompVisual->SetContent(nullptr);
			_dcompDevice->Commit();
		}
	}

	if (_isResized) {
		_isResized = false;
	} else {
		// 确保前一帧渲染完成再渲染下一帧，既降低了 GPU 负载，也能降低延迟
		_WaitForRenderComplete();
	}
}

bool AdaptivePresenter::OnResize() noexcept {
	_isResized = true;

	if (ScalingWindow::Get().IsResizingOrMoving() || !_dxgiSwapChain) {
		// 切换到 DirectComposition 呈现，失败则回落到交换链
		if (_ResizeDCompVisual()) {
			return true;
		}

		Logger::Get().Error("_ResizeDCompVisual 失败");

		// 禁用 DirectFlip 时不存在交换链
		if (!_dxgiSwapChain) {
			return false;
		}
	}

	if (!_ResizeSwapChain()) {
		Logger::Get().Error("_ResizeSwapChain 失败");
		return false;
	}
	
	return true;
}

void AdaptivePresenter::OnEndResize(bool& shouldRedraw) noexcept {
	if (!_isDCompPresenting || !_dxgiSwapChain) {
		shouldRedraw = false;
		return;
	}

	shouldRedraw = true;

	_ResizeSwapChain();
	_isDCompPresenting = false;
	// 交换链呈现新帧后再清除 DirectCompostion 内容，确保无缝切换
	_isSwitchingToSwapChain = true;
}

bool AdaptivePresenter::_ResizeSwapChain() noexcept {
	assert(_dxgiSwapChain);

	if (!_isframeLatencyWaited) {
		_frameLatencyWaitableObject.wait(1000);
		_isframeLatencyWaited = true;
	}

	_backBuffer = nullptr;
	_backBufferRtv = nullptr;

	const RECT& swapChainRect = ScalingWindow::Get().RendererRect();
	const SIZE swapChainSize = Win32Helper::GetSizeOfRect(swapChainRect);
	HRESULT hr = _dxgiSwapChain->ResizeBuffers(
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

	hr = _dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(_backBuffer.put()));
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

	return true;
}

bool AdaptivePresenter::_ResizeDCompVisual(HWND hwndAttach) noexcept {
	const SIZE rendererSize = Win32Helper::GetSizeOfRect(ScalingWindow::Get().RendererRect());

	if (_dcompSurface) {
		// 使用 IDCompositionVirtualSurface 而不是 IDCompositionSurface 的原因是
		// IDCompositionDevice2::CreateSurface 有时相当慢，最坏情况下要几十毫秒。
		HRESULT hr = _dcompSurface->Resize((UINT)rendererSize.cx, (UINT)rendererSize.cy);
		if (FAILED(hr)) {
			Logger::Get().ComError("Resize 失败", hr);
			return false;
		}

		return true;
	}

	// 初始化 DirectComposition
	HRESULT hr = DCompositionCreateDevice3(
		_deviceResources->GetD3DDevice(), IID_PPV_ARGS(&_dcompDevice));
	if (FAILED(hr)) {
		Logger::Get().ComError("DCompositionCreateDevice3 失败", hr);
		return false;
	}

	if (!hwndAttach) {
		// 没有禁用 DirectFlip 时才会在调整大小时初始化，因此必定存在交换链
		hr = _dxgiSwapChain->GetHwnd(&hwndAttach);
		if (FAILED(hr)) {
			Logger::Get().ComError("GetHwnd 失败", hr);
			return false;
		}
	}

	hr = _dcompDevice->CreateTargetForHwnd(hwndAttach, TRUE, _dcompTarget.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateTargetForHwnd 失败", hr);
		return false;
	}

	hr = _dcompDevice->CreateVisual(_dcompVisual.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateVisual 失败", hr);
		return false;
	}

	hr = _dcompTarget->SetRoot(_dcompVisual.get());
	if (FAILED(hr)) {
		Logger::Get().ComError("SetRoot 失败", hr);
		return false;
	}

	hr = _dcompDevice->CreateVirtualSurface(
		(UINT)rendererSize.cx,
		(UINT)rendererSize.cy,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_ALPHA_MODE_IGNORE,
		_dcompSurface.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateVirtualSurface 失败", hr);
		return false;
	}

	hr = _dcompVisual->SetContent(_dcompSurface.get());
	if (FAILED(hr)) {
		Logger::Get().ComError("SetContent 失败", hr);
		// 失败时确保 _dcompSurface 为空
		_dcompSurface = nullptr;
		return false;
	}

	_isDCompPresenting = true;
	return true;
}

}
