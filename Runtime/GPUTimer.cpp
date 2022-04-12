#include "pch.h"
#include "GPUTimer.h"


GPUTimer::GPUTimer() {
	// 这两个函数不会失败
	BOOL success = QueryPerformanceFrequency(&_qpcFrequency);
	assert(success);

	success = QueryPerformanceCounter(&_qpcLastTime);
	assert(success);

	// Initialize max delta to 1/10 of a second.
	_qpcMaxDelta = static_cast<uint64_t>(_qpcFrequency.QuadPart / 10);
}

void GPUTimer::ResetElapsedTime() {
	BOOL success = QueryPerformanceCounter(&_qpcLastTime);
	assert(success);

	_framesPerSecond = 0;
	_framesThisSecond = 0;
	_qpcSecondCounter = 0;
}

void GPUTimer::OnBeginFrame() {
	// Query the current time.
	LARGE_INTEGER currentTime;

	BOOL result = QueryPerformanceCounter(&currentTime);
	assert(result);

	uint64_t timeDelta = static_cast<uint64_t>(currentTime.QuadPart - _qpcLastTime.QuadPart);

	_qpcLastTime = currentTime;
	_qpcSecondCounter += timeDelta;

	// Clamp excessively large time deltas (e.g. after paused in the debugger).
	if (timeDelta > _qpcMaxDelta) {
		timeDelta = _qpcMaxDelta;
	}

	// Convert QPC units into a canonical tick format. This cannot overflow due to the previous clamp.
	timeDelta *= _TICKS_PER_SECOND;
	timeDelta /= static_cast<uint64_t>(_qpcFrequency.QuadPart);

	uint32_t lastFrameCount = _frameCount;

	// Variable timestep update logic.
	_elapsedTicks = timeDelta;
	_totalTicks += timeDelta;
	++_frameCount;

	// Track the current framerate.
	if (_frameCount != lastFrameCount) {
		++_framesThisSecond;
	}

	if (_qpcSecondCounter >= static_cast<uint64_t>(_qpcFrequency.QuadPart)) {
		_framesPerSecond = _framesThisSecond;
		_framesThisSecond = 0;
		_qpcSecondCounter %= static_cast<uint64_t>(_qpcFrequency.QuadPart);
	}
}
