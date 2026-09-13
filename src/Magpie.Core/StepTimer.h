#pragma once

namespace Magpie {

enum class StepTimerStatus {
	WaitingForNewFrame,
	WaitingForFPSLimiter,
	ForceNewFrame
};

class StepTimer {
public:
	StepTimer() = default;

	StepTimer(const StepTimer&) = delete;
	StepTimer(StepTimer&&) = delete;

	void Initialize(float minFrameRate, std::optional<float> maxFrameRate) noexcept;

	StepTimerStatus WaitForNextFrame(bool waitMsgForNewFrame, bool& fpsUpdated) noexcept;

	void PrepareForRender() noexcept;

	uint32_t GetFrameCount() const noexcept {
		return _frameCount;
	}

	// 支持跨线程调用
	uint32_t GetFPS() const noexcept {
		return _framesPerSecond.load(std::memory_order_relaxed);
	}

private:
	bool _HasMinInterval() const noexcept;
	bool _HasMaxInterval() const noexcept;

	void _WaitForMsgAndTimer(std::chrono::nanoseconds time) noexcept;

	bool _UpdateFPS(std::chrono::time_point<std::chrono::steady_clock> now) noexcept;

	std::chrono::nanoseconds _minInterval{};
	std::chrono::nanoseconds _maxInterval{ std::numeric_limits<std::chrono::nanoseconds::rep>::max() };
	wil::unique_event_nothrow _hTimer;

	std::chrono::steady_clock::time_point _thisFrameStartTime;
	std::chrono::steady_clock::time_point _nextFrameStartTime;
	std::chrono::steady_clock::time_point _lastSecondTime;

	uint32_t _frameCount = 0;
	std::atomic<uint32_t> _framesPerSecond = 0;
	uint32_t _framesThisSecond = 0;
};

}
