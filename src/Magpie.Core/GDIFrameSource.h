#pragma once
#include "FrameSourceBase.h"

namespace Magpie::Core {

class GDIFrameSource : public FrameSourceBase {
public:
	virtual ~GDIFrameSource() {}

	bool Initialize(HWND hwndSrc, HWND hwndScaling, const ScalingOptions& options, ID3D11Device5* d3dDevice) noexcept override;

	UpdateState Update() noexcept override;

	bool IsScreenCapture() const noexcept override {
		return false;
	}

	const char* Name() const noexcept override {
		return "GDI";
	}

protected:
	bool _HasRoundCornerInWin11() noexcept override {
		return false;
	}

	bool _CanCaptureTitleBar() noexcept override {
		return false;
	}

private:
	HWND _hwndSrc = NULL;
	RECT _frameRect{};
	winrt::com_ptr<IDXGISurface1> _dxgiSurface;
};

}
