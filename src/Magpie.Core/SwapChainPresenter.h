#pragma once
#include "PresenterBase.h"

namespace Magpie {

class SwapChainPresenter : public PresenterBase {
public:
	bool Initialize(HWND hwndAttach, const DeviceResources& deviceResources) noexcept override;

	ID3D11RenderTargetView* BeginFrame(POINT& updateOffset) noexcept override;

	void EndFrame() noexcept override;

	bool Resize() noexcept override;

private:
	const DeviceResources* _deviceResources = nullptr;

	winrt::com_ptr<IDXGISwapChain4> _swapChain;
	wil::unique_event_nothrow _frameLatencyWaitableObject;
	winrt::com_ptr<ID3D11Texture2D> _backBuffer;
	winrt::com_ptr<ID3D11RenderTargetView> _backBufferRtv;

	winrt::com_ptr<ID3D11Fence> _fence;
	uint64_t _fenceValue = 0;
	wil::unique_event_nothrow _fenceEvent;

	bool _isframeLatencyWaited = false;
	bool _isResized = false;
};

}
