#pragma once
#include "Win32Helper.h"

namespace Magpie {

enum class StepTimerStatus {
	WaitForNewFrame,
	WaitForFPSLimiter,
	ForceNewFrame
};

class StepTimer {
public:
	StepTimer() = default;

	StepTimer(const StepTimer&) = delete;
	StepTimer(StepTimer&&) = delete;

	void Initialize(float minFrameRate, std::optional<float> maxFrameRate) noexcept;

	StepTimerStatus WaitForNextFrame(bool waitMsgForNewFrame) noexcept;

	void UpdateFPS(bool newFrame) noexcept;

	uint32_t FrameCount() const noexcept {
		return _frameCount;
	}

	// 从前端线程调用
	uint32_t FPS() const noexcept {
		return _framesPerSecond.load(std::memory_order_relaxed);
	}

private:
	void _WaitForMsgAndTimer(std::chrono::nanoseconds time) noexcept;

	std::optional<std::chrono::nanoseconds> _minInterval;
	std::chrono::nanoseconds _maxInterval{};
	wil::unique_event_nothrow _hTimer;

	std::chrono::time_point<std::chrono::steady_clock> _lastFrameTime;
	std::chrono::time_point<std::chrono::steady_clock> _lastSecondTime;

	uint32_t _frameCount = 0;
	std::atomic<uint32_t> _framesPerSecond = 0;
	uint32_t _framesThisSecond = 0;

	bool _isWaitingForNewFrame = false;
};

}
