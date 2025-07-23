#pragma once
#include "FrameSourceBase.h"
#include "SmallVector.h"

namespace Magpie {

class DesktopDuplicationFrameSource final : public FrameSourceBase {
public:
	bool Start() noexcept override;

	FrameSourceWaitType WaitType() const noexcept override {
		return FrameSourceWaitType::WaitForFrame;
	}

	const char* Name() const noexcept override {
		return "Desktop Duplication";
	}

protected:
	bool _Initialize() noexcept override;

	FrameSourceState _Update() noexcept override;

private:
	winrt::com_ptr<IDXGIOutput1> _dxgiOutput;
	winrt::com_ptr<IDXGIOutputDuplication> _outputDup;

	SmallVector<uint8_t, 0> _dupMetaData;

	RECT _srcClientInMonitor{};
	D3D11_BOX _frameInMonitor{};

	bool _isFrameAcquired = false;
};

}
