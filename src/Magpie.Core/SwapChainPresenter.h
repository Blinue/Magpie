#pragma once
#include "PresenterBase.h"

namespace Magpie {

class SwapChainPresenter : public PresenterBase {
protected:
	bool _Initialize(HWND hwndAttach) noexcept override;

	void _Present() noexcept override;

	bool _Resize() noexcept override;

public:
	bool BeginFrame(
		winrt::com_ptr<ID3D11Texture2D>& frameTex,
		winrt::com_ptr<ID3D11RenderTargetView>& frameRtv,
		POINT& drawOffset
	) noexcept override;

private:
	winrt::com_ptr<IDXGISwapChain4> _dxgiSwapChain;
	wil::unique_event_nothrow _frameLatencyWaitableObject;
	winrt::com_ptr<ID3D11Texture2D> _backBuffer;
	winrt::com_ptr<ID3D11RenderTargetView> _backBufferRtv;

	bool _isframeLatencyWaited = false;
};

}
