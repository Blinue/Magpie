#pragma once
#include "FrameSourceBase.h"
#include "Win32Utils.h"
#include "SmallVector.h"

namespace Magpie::Core {

class DesktopDuplicationFrameSource final : public FrameSourceBase {
public:
	bool IsScreenCapture() const noexcept override {
		return true;
	}

	FrameSourceWaitType WaitType() const noexcept override {
		return WaitForFrame;
	}

	const char* Name() const noexcept override {
		return "Desktop Duplication";
	}

protected:
	bool _HasRoundCornerInWin11() noexcept override {
		return true;
	}

	bool _CanCaptureTitleBar() noexcept override {
		return true;
	}

	bool _Initialize() noexcept override;

	UpdateState _Update() noexcept override;

private:
	winrt::com_ptr<IDXGIOutputDuplication> _outputDup;

	SmallVector<uint8_t, 0> _dupMetaData;

	RECT _srcClientInMonitor{};
	D3D11_BOX _frameInMonitor{};

	bool _isFrameAcquired = false;
};

}
