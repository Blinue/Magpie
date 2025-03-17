#pragma once
#include "PresenterBase.h"
#include <dcomp.h>

namespace Magpie {

class DCompPresenter : public PresenterBase {
protected:
	bool _Initialize(HWND hwndAttach) noexcept override;

public:
	winrt::com_ptr<ID3D11RenderTargetView> BeginFrame(POINT& updateOffset) noexcept override;

	void EndFrame() noexcept override;

	bool Resize() noexcept override;

private:
	bool _CreateSurface() noexcept;

	winrt::com_ptr<IDCompositionDesktopDevice> _device;
	winrt::com_ptr<IDCompositionTarget> _target;
	winrt::com_ptr<IDCompositionVisual2> _visual;
	winrt::com_ptr<IDCompositionSurface> _surface;

	bool _isResized = false;
};

}
