#include "pch.h"
#include "StepTimer.h"

using namespace std::chrono;

namespace Magpie {

void StepTimer::Initialize(float minFrameRate, std::optional<float> maxFrameRate) noexcept {
	if (minFrameRate > 1e-6) {
		_maxInterval = duration_cast<nanoseconds>(duration<float>(1 / minFrameRate));
	} else {
		_maxInterval = nanoseconds(std::numeric_limits<nanoseconds::rep>::max());
	}

	if (maxFrameRate) {
		_minInterval = duration_cast<nanoseconds>(duration<float>(1 / *maxFrameRate));
	}

	_hTimer.reset(CreateWaitableTimerEx(nullptr, nullptr,
		CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS));
}

StepTimerStatus StepTimer::WaitForNextFrame(bool waitMsgForNewFrame) noexcept {
	const time_point<steady_clock> now = steady_clock::now();

	if (_lastFrameTime == time_point<steady_clock>{}) {
		_lastFrameTime = now;
		return StepTimerStatus::WaitForNewFrame;
	}

	const nanoseconds delta = now - _lastFrameTime;

	if (delta >= _maxInterval) {
		_lastFrameTime = now - delta % _maxInterval;
		return StepTimerStatus::ForceNewFrame;
	}

	if (_minInterval) {
		if (delta < *_minInterval) {
			_WaitForMsgAndTimer(*_minInterval - delta);
			return StepTimerStatus::WaitForFPSLimiter;
		}

		if (!_isWaitingForNewFrame) {
			_isWaitingForNewFrame = true;
			_lastFrameTime = now - delta % *_minInterval;
		}
	}

	if (waitMsgForNewFrame) {
		if (_maxInterval == nanoseconds(std::numeric_limits<nanoseconds::rep>::max())) {
			WaitMessage();
		} else {
			_WaitForMsgAndTimer(_maxInterval - delta);
		}
	}

	return StepTimerStatus::WaitForNewFrame;
}

void StepTimer::UpdateFPS(bool newFrame) noexcept {
	if (newFrame) {
		_isWaitingForNewFrame = false;
	}

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

void StepTimer::_WaitForMsgAndTimer(std::chrono::nanoseconds time) noexcept {
	if (time > 1ms) {
		// Sleep 精度太低，我们使用 WaitableTimer 睡眠。负值表示相对时间
		LARGE_INTEGER liDueTime{
			.QuadPart = (time - 1ms).count() / -100
		};
		SetWaitableTimerEx(_hTimer.get(), &liDueTime, 0, NULL, NULL, 0, 0);

		// 新消息到达则中止等待
		HANDLE hTimer = _hTimer.get();
		MsgWaitForMultipleObjectsEx(1, &hTimer, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
	} else {
		// 剩余时间在 1ms 以内则“忙等待”
		Sleep(0);
	}
}

}
