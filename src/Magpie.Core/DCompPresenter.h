#pragma once
#include "PresenterBase.h"
#include <dcomp.h>

namespace Magpie {

class DCompPresenter : public PresenterBase {
protected:
	bool _Initialize(HWND hwndAttach) noexcept override;

	void _EndDraw() noexcept override;

	void _Present() noexcept override;

	bool _Resize() noexcept override;

public:
	bool BeginFrame(
		winrt::com_ptr<ID3D11Texture2D>& frameTex,
		winrt::com_ptr<ID3D11RenderTargetView>& frameRtv,
		POINT& drawOffset
	) noexcept override;

private:
	bool _CreateSurface() noexcept;

	winrt::com_ptr<IDCompositionDesktopDevice> _device;
	winrt::com_ptr<IDCompositionTarget> _target;
	winrt::com_ptr<IDCompositionVisual2> _visual;
	winrt::com_ptr<IDCompositionSurface> _surface;
};

}
