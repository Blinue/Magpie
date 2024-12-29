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
		assert(_minInterval <= _maxInterval);
	}

	if (_HasMinFrameRate() || maxFrameRate) {
		_hTimer.reset(CreateWaitableTimerEx(nullptr, nullptr,
			CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS));
	}
}

StepTimerStatus StepTimer::WaitForNextFrame(bool waitMsgForNewFrame) noexcept {
	const time_point<steady_clock> now = steady_clock::now();

	if (_lastFrameTime == time_point<steady_clock>{}) {
		// 等待第一帧
		if (waitMsgForNewFrame) {
			WaitMessage();
		}
		return StepTimerStatus::WaitForNewFrame;
	}

	const nanoseconds delta = now - _lastFrameTime;

	if (delta >= _maxInterval) {
		return StepTimerStatus::ForceNewFrame;
	}

	if (_minInterval) {
		if (delta < *_minInterval) {
			_WaitForMsgAndTimer(*_minInterval - delta);
			UpdateFPS(false);
			return StepTimerStatus::WaitForFPSLimiter;
		}
	}

	if (waitMsgForNewFrame) {
		if (_HasMinFrameRate()) {
			_WaitForMsgAndTimer(_maxInterval - delta);
		} else {
			WaitMessage();
		}
	}

	return StepTimerStatus::WaitForNewFrame;
}

void StepTimer::UpdateFPS(bool newFrame) noexcept {
	if (newFrame) {
		const time_point<steady_clock> now = steady_clock::now();
		if (_minInterval) {
			_lastFrameTime = now - (now - _lastFrameTime) % *_minInterval;
		} else {
			_lastFrameTime = now;
		}
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

bool StepTimer::_HasMinFrameRate() const noexcept {
	return _maxInterval != nanoseconds(std::numeric_limits<nanoseconds::rep>::max());
}

}
