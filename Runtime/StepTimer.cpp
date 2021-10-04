#include "pch.h"
#include "StepTimer.h"


StepTimer::StepTimer() {
	// 这两个函数不会失败
	BOOL success = QueryPerformanceFrequency(&m_qpcFrequency);
	assert(success);

	success = QueryPerformanceCounter(&m_qpcLastTime);
	assert(success);

	// Initialize max delta to 1/10 of a second.
	m_qpcMaxDelta = static_cast<uint64_t>(m_qpcFrequency.QuadPart / 10);
}

void StepTimer::ResetElapsedTime() {
	BOOL success = QueryPerformanceCounter(&m_qpcLastTime);
	assert(success);

	m_leftOverTicks = 0;
	m_framesPerSecond = 0;
	m_framesThisSecond = 0;
	m_qpcSecondCounter = 0;
}

void StepTimer::Tick(std::function<void()> render) {
    // Query the current time.
    LARGE_INTEGER currentTime;

    BOOL result = QueryPerformanceCounter(&currentTime);
    assert(result);

    uint64_t timeDelta = static_cast<uint64_t>(currentTime.QuadPart - m_qpcLastTime.QuadPart);

    m_qpcLastTime = currentTime;
    m_qpcSecondCounter += timeDelta;

    // Clamp excessively large time deltas (e.g. after paused in the debugger).
    if (timeDelta > m_qpcMaxDelta) {
        timeDelta = m_qpcMaxDelta;
    }

    // Convert QPC units into a canonical tick format. This cannot overflow due to the previous clamp.
    timeDelta *= TicksPerSecond;
    timeDelta /= static_cast<uint64_t>(m_qpcFrequency.QuadPart);

    uint32_t lastFrameCount = m_frameCount;

    if (m_isFixedTimeStep) {
        // Fixed timestep update logic

        // If the app is running very close to the target elapsed time (within 1/4 of a millisecond) just clamp
        // the clock to exactly match the target value. This prevents tiny and irrelevant errors
        // from accumulating over time. Without this clamping, a game that requested a 60 fps
        // fixed update, running with vsync enabled on a 59.94 NTSC display, would eventually
        // accumulate enough tiny errors that it would drop a frame. It is better to just round
        // small deviations down to zero to leave things running smoothly.

        if (static_cast<uint64_t>(std::abs(static_cast<int64_t>(timeDelta - m_targetElapsedTicks))) < TicksPerSecond / 4000) {
            timeDelta = m_targetElapsedTicks;
        }

        m_leftOverTicks += timeDelta;

        if (m_leftOverTicks >= m_targetElapsedTicks) {
            m_elapsedTicks = m_targetElapsedTicks;
            m_totalTicks += m_targetElapsedTicks;
            m_leftOverTicks %= m_targetElapsedTicks;

            m_frameCount++;
            render();
        }
    } else {
        // Variable timestep update logic.
        m_elapsedTicks = timeDelta;
        m_totalTicks += timeDelta;
        m_leftOverTicks = 0;
        m_frameCount++;

        render();
    }

    // Track the current framerate.
    if (m_frameCount != lastFrameCount) {
        m_framesThisSecond++;
    }

    if (m_qpcSecondCounter >= static_cast<uint64_t>(m_qpcFrequency.QuadPart)) {
        m_framesPerSecond = m_framesThisSecond;
        m_framesThisSecond = 0;
        m_qpcSecondCounter %= static_cast<uint64_t>(m_qpcFrequency.QuadPart);
    }
}
