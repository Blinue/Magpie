#pragma once
#include "PresenterBase.h"

namespace Magpie {

class SwapChainPresenter : public PresenterBase {
protected:
	bool _Initialize(HWND hwndAttach) noexcept override;

public:
	winrt::com_ptr<ID3D11RenderTargetView> BeginFrame(POINT& updateOffset) noexcept override;

	void EndFrame() noexcept override;

	bool Resize() noexcept override;

private:
	winrt::com_ptr<IDXGISwapChain4> _swapChain;
	wil::unique_event_nothrow _frameLatencyWaitableObject;
	winrt::com_ptr<ID3D11Texture2D> _backBuffer;
	winrt::com_ptr<ID3D11RenderTargetView> _backBufferRtv;

	bool _isframeLatencyWaited = false;
	bool _isResized = false;
};

}
