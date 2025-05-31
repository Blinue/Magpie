#pragma once
#include "FrameSourceBase.h"

namespace Magpie {

class GDIFrameSource final : public FrameSourceBase {
public:
	virtual ~GDIFrameSource() {}

	FrameSourceWaitType WaitType() const noexcept override {
		return FrameSourceWaitType::NoWait;
	}

	const char* Name() const noexcept override {
		return "GDI";
	}

protected:
	bool _Initialize() noexcept override;

	FrameSourceState _Update() noexcept override;

private:
	RECT _frameRect{};
	winrt::com_ptr<IDXGISurface1> _dxgiSurface;
};

}
