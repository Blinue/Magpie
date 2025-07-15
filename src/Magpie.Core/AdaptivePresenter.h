#pragma once
#include "PresenterBase.h"
#include <dcomp.h>

namespace Magpie {

// 根据需要在交换链和 DirectComposition 两种呈现方式间切换。交换链可以触发
// DirectFlip/IndependentFlip 以最小化延迟，DirectComposition 在调整尺寸
// 时闪烁更少，这个呈现器旨在结合两者的优势。
class AdaptivePresenter : public PresenterBase {
protected:
	bool _Initialize(HWND hwndAttach) noexcept override;

public:
	bool BeginFrame(
		winrt::com_ptr<ID3D11Texture2D>& frameTex,
		winrt::com_ptr<ID3D11RenderTargetView>& frameRtv,
		POINT& drawOffset
	) noexcept override;

	void EndFrame(bool waitForRenderComplete = false) noexcept override;

	bool OnResize() noexcept override;

	void OnEndResize(bool& shouldRedraw) noexcept override;

private:
	bool _ResizeSwapChain() noexcept;

	bool _ResizeDCompVisual(HWND hwndAttach = NULL) noexcept;

	winrt::com_ptr<IDXGISwapChain4> _dxgiSwapChain;
	wil::unique_event_nothrow _frameLatencyWaitableObject;
	winrt::com_ptr<ID3D11Texture2D> _backBuffer;
	winrt::com_ptr<ID3D11RenderTargetView> _backBufferRtv;

	// 调整大小或禁用 DirectFlip 时使用
	winrt::com_ptr<IDCompositionDesktopDevice> _dcompDevice;
	winrt::com_ptr<IDCompositionTarget> _dcompTarget;
	winrt::com_ptr<IDCompositionVisual2> _dcompVisual;
	winrt::com_ptr<IDCompositionVirtualSurface> _dcompSurface;
	
	bool _isDCompPresenting = false;
	bool _isResized = false;
	bool _isframeLatencyWaited = false;
	bool _isSwitchingToSwapChain = false;
};

}
