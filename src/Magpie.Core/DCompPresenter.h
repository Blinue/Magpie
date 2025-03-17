#pragma once
#include "PresenterBase.h"
#include <dcomp.h>

namespace Magpie {

class DCompPresenter : public PresenterBase {
public:
	bool Initialize(HWND hwndAttach, const DeviceResources& deviceResources) noexcept override;

	winrt::com_ptr<ID3D11RenderTargetView> BeginFrame(POINT& updateOffset) noexcept override;

	void EndFrame() noexcept override;

	bool Resize() noexcept override;

private:
	const DeviceResources* _deviceResources = nullptr;

	winrt::com_ptr<IDCompositionDesktopDevice> _device;
	winrt::com_ptr<IDCompositionTarget> _target;
	winrt::com_ptr<IDCompositionVisual2> _visual;
	winrt::com_ptr<IDCompositionSurface> _surface;

	winrt::com_ptr<ID3D11Fence> _fence;
	uint64_t _fenceValue = 0;
	wil::unique_event_nothrow _fenceEvent;

	bool _isResized = false;
};

}
