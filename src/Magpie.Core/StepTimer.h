#pragma once
#include "Win32Helper.h"

namespace Magpie::Core {

class StepTimer {
public:
	StepTimer() = default;

	StepTimer(const StepTimer&) = delete;
	StepTimer(StepTimer&&) = delete;

	void Initialize(std::optional<float> maxFrameRate) noexcept;

	bool WaitForNextFrame() noexcept;

	void UpdateFPS(bool newFrame) noexcept;

	uint32_t FrameCount() const noexcept {
		return _frameCount;
	}

	// 从前端线程调用
	uint32_t FPS() const noexcept {
		return _framesPerSecond.load(std::memory_order_relaxed);
	}

private:
	std::optional<std::chrono::nanoseconds> _minInterval;
	wil::unique_event_nothrow _hTimer;

	std::chrono::time_point<std::chrono::steady_clock> _lastFrameTime;
	std::chrono::time_point<std::chrono::steady_clock> _lastSecondTime;

	uint32_t _frameCount = 0;
	std::atomic<uint32_t> _framesPerSecond = 0;
	uint32_t _framesThisSecond = 0;
};

}
