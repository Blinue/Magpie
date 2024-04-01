#pragma once
#include "FrameSourceBase.h"

namespace Magpie::Core {

class DwmSharedSurfaceFrameSource final : public FrameSourceBase {
public:
	virtual ~DwmSharedSurfaceFrameSource() {}

	bool IsScreenCapture() const noexcept override {
		return false;
	}

	bool CanWaitForFrame() const noexcept override {
		return false;
	}

	const char* Name() const noexcept override {
		return "DwmSharedSurface";
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
	D3D11_BOX _frameInWnd{};
};

}
