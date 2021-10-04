//
// StepTimer.h - A simple timer that provides elapsed time information
//

#pragma once

#include "pch.h"


// 帧率限制器
class StepTimer {
public:
	StepTimer();

	// Get elapsed time since the previous Update call.
	uint64_t GetElapsedTicks() const noexcept { return m_elapsedTicks; }
	double GetElapsedSeconds() const noexcept { return TicksToSeconds(m_elapsedTicks); }

	// Get total time since the start of the program.
	uint64_t GetTotalTicks() const noexcept { return m_totalTicks; }
	double GetTotalSeconds() const noexcept { return TicksToSeconds(m_totalTicks); }

	// Get total number of updates since start of the program.
	uint32_t GetFrameCount() const noexcept { return m_frameCount; }

	// Get the current framerate.
	uint32_t GetFramesPerSecond() const noexcept { return m_framesPerSecond; }

	// Set whether to use fixed or variable timestep mode.
	void SetFixedTimeStep(bool isFixedTimestep) noexcept { m_isFixedTimeStep = isFixedTimestep; }

	// Set how often to call Update when in fixed timestep mode.
	void SetTargetElapsedTicks(uint64_t targetElapsed) noexcept { m_targetElapsedTicks = targetElapsed; }
	void SetTargetElapsedSeconds(double targetElapsed) noexcept { m_targetElapsedTicks = SecondsToTicks(targetElapsed); }

	// Integer format represents time using 10,000,000 ticks per second.
	static constexpr uint64_t TicksPerSecond = 10000000;

	static constexpr double TicksToSeconds(uint64_t ticks) noexcept { return static_cast<double>(ticks) / TicksPerSecond; }
	static constexpr uint64_t SecondsToTicks(double seconds) noexcept { return static_cast<uint64_t>(seconds * TicksPerSecond); }

	// After an intentional timing discontinuity (for instance a blocking IO operation)
	// call this to avoid having the fixed timestep logic attempt a set of catch-up
	// Update calls.

	void ResetElapsedTime();

	// Update timer state, calling the specified Update function the appropriate number of times.
	void Tick(std::function<void()> render);

private:
	// Source timing data uses QPC units.
	LARGE_INTEGER m_qpcFrequency{};
	LARGE_INTEGER m_qpcLastTime{};
	uint64_t m_qpcMaxDelta = 0;

	// Derived timing data uses a canonical tick format.
	uint64_t m_elapsedTicks = 0;
	uint64_t m_totalTicks = 0;
	uint64_t m_leftOverTicks = 0;

	// Members for tracking the framerate.
	uint32_t m_frameCount = 0;
	uint32_t m_framesPerSecond = 0;
	uint32_t m_framesThisSecond = 0;
	uint64_t m_qpcSecondCounter = 0;

	// Members for configuring fixed timestep mode.
	bool m_isFixedTimeStep = false;
	uint64_t m_targetElapsedTicks = TicksPerSecond / 60;
};
