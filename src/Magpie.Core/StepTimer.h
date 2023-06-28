#pragma once
#include "Win32Utils.h"

namespace Magpie::Core {

class StepTimer {
public:
	StepTimer() = default;

	StepTimer(const StepTimer&) = delete;
	StepTimer(StepTimer&&) = delete;

	void Initialize(std::optional<float> maxFrameRate) noexcept;

	bool NewFrame(bool isDupFrame) noexcept;

	uint32_t FrameCount() const noexcept {
		return _frameCount;
	}

private:
	std::optional<std::chrono::nanoseconds> _minInterval;

	std::chrono::time_point<std::chrono::steady_clock> _lastFrameTime;
	// 上一帧的渲染时间
	std::chrono::nanoseconds _elapsedTime{};

	uint32_t _frameCount = 0;
	uint32_t _framesPerSecond = 0;
	uint32_t _framesThisSecond = 0;
	std::chrono::nanoseconds _fpsCounter{};

	Win32Utils::ScopedHandle _hTimer;

};

}
