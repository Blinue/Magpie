#pragma once
#include "pch.h"


// 用于记录帧率和 GPU 时间
class GPUTimer {
public:
	GPUTimer();

	// Get elapsed time since the previous Update call.
	double GetElapsedSeconds() const noexcept { return _TicksToSeconds(_elapsedTicks); }

	// Get total time since the start of the program.
	double GetTotalSeconds() const noexcept { return _TicksToSeconds(_totalTicks); }

	// Get total number of updates since start of the program.
	uint32_t GetFrameCount() const noexcept { return _frameCount; }

	// Get the current framerate.
	uint32_t GetFramesPerSecond() const noexcept { return _framesPerSecond; }

	// After an intentional timing discontinuity (for instance a blocking IO operation)
	// call this to avoid having the fixed timestep logic attempt a set of catch-up
	// Update calls.

	void ResetElapsedTime();

	// 在每帧开始时调用，用于记录帧率
	void OnBeginFrame();

private:
	// Integer format represents time using 10,000,000 ticks per second.
	static constexpr uint64_t _TICKS_PER_SECOND = 10000000;

	static constexpr double _TicksToSeconds(uint64_t ticks) noexcept { return static_cast<double>(ticks) / _TICKS_PER_SECOND; }
	static constexpr uint64_t _SecondsToTicks(double seconds) noexcept { return static_cast<uint64_t>(seconds * _TICKS_PER_SECOND); }

	// Source timing data uses QPC units.
	LARGE_INTEGER _qpcFrequency{};
	LARGE_INTEGER _qpcLastTime{};
	uint64_t _qpcMaxDelta = 0;

	// Derived timing data uses a canonical tick format.
	uint64_t _elapsedTicks = 0;
	uint64_t _totalTicks = 0;
	uint64_t _lastFrameTicks = 0;

	// Members for tracking the framerate.
	uint32_t _frameCount = 0;
	uint32_t _framesPerSecond = 0;
	uint32_t _framesThisSecond = 0;
	uint64_t _qpcSecondCounter = 0;
};
