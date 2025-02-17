#pragma once
#include "FrameSourceBase.h"

namespace Magpie {

class DwmSharedSurfaceFrameSource final : public FrameSourceBase {
public:
	virtual ~DwmSharedSurfaceFrameSource() {}

	bool IsScreenCapture() const noexcept override {
		return false;
	}

	FrameSourceWaitType WaitType() const noexcept override {
		return FrameSourceWaitType::NoWait;
	}

	const char* Name() const noexcept override {
		return "DwmSharedSurface";
	}

protected:
	bool _Initialize() noexcept override;

	FrameSourceState _Update() noexcept override;

	bool _HasRoundCornerInWin11() noexcept override {
		return false;
	}

private:
	D3D11_BOX _frameInWnd{};
};

}
