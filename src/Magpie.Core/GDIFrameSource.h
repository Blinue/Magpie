#pragma once
#include "FrameSourceBase.h"

namespace Magpie::Core {

class GDIFrameSource final : public FrameSourceBase {
public:
	virtual ~GDIFrameSource() {}

	bool IsScreenCapture() const noexcept override {
		return false;
	}

	const char* Name() const noexcept override {
		return "GDI";
	}

protected:
	bool _Initialize() noexcept override;

	UpdateState _Update() noexcept override;

	bool _HasRoundCornerInWin11() noexcept override {
		return false;
	}

	bool _CanCaptureTitleBar() noexcept override {
		return false;
	}

private:
	RECT _frameRect{};
	winrt::com_ptr<IDXGISurface1> _dxgiSurface;
};

}
