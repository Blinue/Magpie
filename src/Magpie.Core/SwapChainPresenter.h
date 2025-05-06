#pragma once
#include "PresenterBase.h"
#include <dcomp.h>

namespace Magpie {

class SwapChainPresenter : public PresenterBase {
protected:
	bool _Initialize(HWND hwndAttach) noexcept override;

public:
	bool BeginFrame(
		winrt::com_ptr<ID3D11Texture2D>& frameTex,
		winrt::com_ptr<ID3D11RenderTargetView>& frameRtv,
		POINT& drawOffset
	) noexcept override;

	void EndFrame() noexcept override;

	bool Resize() noexcept override;

	void EndResize(bool& shouldRedraw) noexcept override;

private:
	bool _ResizeSwapChain() noexcept;

	winrt::com_ptr<IDXGISwapChain4> _dxgiSwapChain;
	wil::unique_event_nothrow _frameLatencyWaitableObject;
	winrt::com_ptr<ID3D11Texture2D> _backBuffer;
	winrt::com_ptr<ID3D11RenderTargetView> _backBufferRtv;

	// 调整大小或禁用 DirectFlip 时使用
	winrt::com_ptr<IDCompositionDesktopDevice> _dcompDevice;
	winrt::com_ptr<IDCompositionTarget> _dcompTarget;
	winrt::com_ptr<IDCompositionVisual2> _dcompVisual;
	winrt::com_ptr<IDCompositionSurface> _dcompSurface;

	bool _isResized = false;
	bool _isframeLatencyWaited = false;
	bool _shouldSwitchToSwapChain = false;
};

}
