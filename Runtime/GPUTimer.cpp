#include "pch.h"
#include "GPUTimer.h"


GPUTimer::GPUTimer() {
	// 这两个函数不会失败
	BOOL success = QueryPerformanceFrequency(&m_qpcFrequency);
	assert(success);

	success = QueryPerformanceCounter(&m_qpcLastTime);
	assert(success);

	// Initialize max delta to 1/10 of a second.
	m_qpcMaxDelta = static_cast<uint64_t>(m_qpcFrequency.QuadPart / 10);
}

void GPUTimer::ResetElapsedTime() {
	BOOL success = QueryPerformanceCounter(&m_qpcLastTime);
	assert(success);

	m_framesPerSecond = 0;
	m_framesThisSecond = 0;
	m_qpcSecondCounter = 0;
}

void GPUTimer::BeginFrame() {
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
	timeDelta *= _TICKS_PER_SECOND;
	timeDelta /= static_cast<uint64_t>(m_qpcFrequency.QuadPart);

	uint32_t lastFrameCount = m_frameCount;

	// Variable timestep update logic.
	m_elapsedTicks = timeDelta;
	m_totalTicks += timeDelta;
	++m_frameCount;

	// Track the current framerate.
	if (m_frameCount != lastFrameCount) {
		++m_framesThisSecond;
	}

	if (m_qpcSecondCounter >= static_cast<uint64_t>(m_qpcFrequency.QuadPart)) {
		m_framesPerSecond = m_framesThisSecond;
		m_framesThisSecond = 0;
		m_qpcSecondCounter %= static_cast<uint64_t>(m_qpcFrequency.QuadPart);

		OutputDebugString(fmt::format(L"{}\n", m_framesPerSecond).c_str());
	}
}
