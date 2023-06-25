#include "pch.h"
#include "StepTimer.h"

using namespace std::chrono;

namespace Magpie::Core {

void StepTimer::Initialize(std::optional<float> maxFrameRate) noexcept {
	if (maxFrameRate) {
		_minInterval = duration_cast<nanoseconds>(duration<float>(1 / *maxFrameRate));
	}
}

bool StepTimer::NewFrame() noexcept {
	auto now = high_resolution_clock::now();

	if (_minInterval) {
		const nanoseconds delta = now - _lastFrameTime;
		if (delta >= *_minInterval) {
			_elapsedTime = delta;
		} else {
			return false;
		}
	} else {
		_elapsedTime = now - _lastFrameTime;
	}
	
	_lastFrameTime = now;

	// 更新当前帧率
	++_framesThisSecond;
	++_frameCount;

	_fpsCounter += _elapsedTime;
	if (_fpsCounter >= 1s) {
		_framesPerSecond = _framesThisSecond;
		_framesThisSecond = 0;
		_fpsCounter %= 1s;

		OutputDebugString(fmt::format(L"fps: {}\n", _framesPerSecond).c_str());
	}

	return true;
}

}
