#include "pch.h"
#include "StepTimer.h"

using namespace std::chrono;

namespace Magpie::Core {

void StepTimer::Initialize(std::optional<float> maxFrameRate) noexcept {
	if (maxFrameRate) {
		_minInterval = duration_cast<nanoseconds>(duration<float>(1 / *maxFrameRate));
		_hTimer.reset(CreateWaitableTimerEx(nullptr, nullptr,
			CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS));
	}
}

bool StepTimer::NewFrame() noexcept {
	const auto now = high_resolution_clock::now();
	const nanoseconds delta = now - _lastFrameTime;

	if (_minInterval && delta < *_minInterval) {
		const nanoseconds rest = *_minInterval - delta;
		if (rest > 1ms) {
			// Sleep 精度太低，我们使用 WaitableTimer 睡眠
			LARGE_INTEGER liDueTime;
			liDueTime.QuadPart = (rest - 1ms).count() / -100;
			SetWaitableTimerEx(_hTimer.get(), &liDueTime, 0, NULL, NULL, 0, 0);
			WaitForSingleObject(_hTimer.get(), INFINITE);
		} else {
			// 剩余时间在 1ms 以内则“忙等待”
			Sleep(0);
		}

		return false;
	}
	
	_elapsedTime = delta;
	_lastFrameTime = now;

	// 更新当前帧率
	++_framesThisSecond;
	++_frameCount;

	_fpsCounter += delta;
	if (_fpsCounter >= 1s) {
		_framesPerSecond = _framesThisSecond;
		_framesThisSecond = 0;
		_fpsCounter %= 1s;

		OutputDebugString(fmt::format(L"fps: {}\n", _framesPerSecond).c_str());
	}

	return true;
}

}
