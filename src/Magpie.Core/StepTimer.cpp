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
	const auto now = steady_clock::now();
	const nanoseconds delta = now - _lastFrameTime;

	if (_minInterval && delta < *_minInterval) {
		if (*_minInterval - delta > 1ms) {
			// Sleep 精度太低，我们使用 WaitableTimer 睡眠。每次只睡眠 1ms，长时间睡眠会使精度下降
			// 负值表示相对时间
			LARGE_INTEGER liDueTime{
				.QuadPart = -10000
			};
			SetWaitableTimerEx(_hTimer.get(), &liDueTime, 0, NULL, NULL, 0, 0);
			WaitForSingleObject(_hTimer.get(), INFINITE);
		} else {
			// 剩余时间在 1ms 以内则“忙等待”
			Sleep(0);
		}

		return false;
	}

	_lastFrameTime = _minInterval ? now - delta % *_minInterval : now;

	// 更新当前帧率
	++_framesThisSecond;
	++_frameCount;

	_fpsCounter += delta;
	if (_fpsCounter >= 1s) {
		_framesPerSecond.store(_framesThisSecond, std::memory_order_relaxed);
		_framesThisSecond = 0;
		_fpsCounter %= 1s;
	}

	return true;
}

}
