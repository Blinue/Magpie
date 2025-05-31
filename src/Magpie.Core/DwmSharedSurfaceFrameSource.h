#pragma once
#include "FrameSourceBase.h"

namespace Magpie {

class DwmSharedSurfaceFrameSource final : public FrameSourceBase {
public:
	virtual ~DwmSharedSurfaceFrameSource() {}

	FrameSourceWaitType WaitType() const noexcept override {
		return FrameSourceWaitType::NoWait;
	}

	const char* Name() const noexcept override {
		return "DwmSharedSurface";
	}

protected:
	bool _Initialize() noexcept override;

	FrameSourceState _Update() noexcept override;

private:
	D3D11_BOX _frameInWnd{};
};

}
