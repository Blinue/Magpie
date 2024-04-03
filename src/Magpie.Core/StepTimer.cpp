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

bool StepTimer::WaitForNextFrame() noexcept {
	if (!_minInterval) {
		return true;
	}

	const time_point<steady_clock> now = steady_clock::now();
	const nanoseconds delta = now - _lastFrameTime;
	if (delta >= *_minInterval) {
		_lastFrameTime = now - delta % *_minInterval;
		return true;
	}

	const nanoseconds rest = *_minInterval - delta;
	if (rest > 1ms) {
		// Sleep 精度太低，我们使用 WaitableTimer 睡眠。负值表示相对时间
		LARGE_INTEGER liDueTime{
			.QuadPart = (rest - 1ms).count() / -100
		};
		SetWaitableTimerEx(_hTimer.get(), &liDueTime, 0, NULL, NULL, 0, 0);
		WaitForSingleObject(_hTimer.get(), INFINITE);
	} else {
		// 剩余时间在 1ms 以内则“忙等待”
		Sleep(0);
	}

	return false;
}

void StepTimer::UpdateFPS(bool newFrame) noexcept {
	if (_lastSecondTime == time_point<steady_clock>{}) {
		// 第一帧
		if (!newFrame) {
			// 在第一帧前不更新 FPS
			return;
		}

		_lastSecondTime = steady_clock::now();
		_framesPerSecond.store(1, std::memory_order_relaxed);
		return;
	}

	if (newFrame) {
		// 更新帧数
		++_framesThisSecond;
		++_frameCount;
	}

	const time_point<steady_clock> now = steady_clock::now();
	const nanoseconds delta = now - _lastSecondTime;
	if (delta >= 1s) {
		_lastSecondTime = now - delta % 1s;

		_framesPerSecond.store(_framesThisSecond, std::memory_order_relaxed);
		_framesThisSecond = 0;
	}
	
}

}
