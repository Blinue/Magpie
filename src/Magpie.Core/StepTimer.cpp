#include "pch.h"
#include "StepTimer.h"

using namespace std::chrono;

namespace Magpie {

void StepTimer::Initialize(float minFrameRate, std::optional<float> maxFrameRate) noexcept {
	assert(minFrameRate >= 0);
	if (minFrameRate > 0) {
		_maxInterval = duration_cast<nanoseconds>(duration<float>(1 / minFrameRate));
	}

	if (maxFrameRate) {
		assert(*maxFrameRate > 0);
		_minInterval = duration_cast<nanoseconds>(duration<float>(1 / *maxFrameRate));
		assert(_minInterval <= _maxInterval);

		if (_HasMaxInterval()) {
			// 确保最大帧间隔是最小帧间隔的整数倍，这能使帧间隔保持稳定，代价是实际最小帧率可能比要求的稍高一点
			_maxInterval = _maxInterval / _minInterval * _minInterval;
		}
	}
}

StepTimerStatus StepTimer::WaitForNextFrame(bool waitMsgForNewFrame) noexcept {
	const time_point<steady_clock> now = steady_clock::now();

	if (_lastFrameTime == time_point<steady_clock>{}) {
		// 等待第一帧，无需更新 FPS
		if (waitMsgForNewFrame) {
			WaitMessage();
		}

		if (_isNewFrame) {
			_isNewFrame = false;
			_lastFrameTime = now;
		}

		return StepTimerStatus::WaitForNewFrame;
	}

	if (_isNewFrame) {
		_isNewFrame = false;

		if (_HasMinInterval()) {
			// 总是以最小帧间隔计算上一帧的时间点。因为最大帧间隔是最小帧间隔的整数倍，
			// 帧间隔可以保持稳定。
			_lastFrameTime = now - (now - _lastFrameTime) % _minInterval;
		} else {
			_lastFrameTime = now;
		}
	}

	const nanoseconds delta = now - _lastFrameTime;

	if (delta >= _maxInterval) {
		return StepTimerStatus::ForceNewFrame;
	}

	// 没有新帧也应更新 FPS。作为性能优化，强制帧无需更新，因为 PrepareForNewFrame 必定会执行
	_UpdateFPS(now);

	if (delta < _minInterval) {
		_WaitForMsgAndTimer(_minInterval - delta);
		return StepTimerStatus::WaitForFPSLimiter;
	}

	if (waitMsgForNewFrame) {
		if (_HasMaxInterval()) {
			_WaitForMsgAndTimer(_maxInterval - delta);
		} else {
			// 没有最小帧率限制则只需等待消息
			WaitMessage();
		}
	}

	return StepTimerStatus::WaitForNewFrame;
}

void StepTimer::PrepareForNewFrame() noexcept {
	_isNewFrame = true;

	++_framesThisSecond;
	++_frameCount;

	_UpdateFPS(steady_clock::now());
}

void StepTimer::_WaitForMsgAndTimer(std::chrono::nanoseconds time) noexcept {
	if (time > 1ms) {
		if (!_hTimer) {
			_hTimer.reset(CreateWaitableTimerEx(nullptr, nullptr,
				CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS));
		}

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

void StepTimer::_UpdateFPS(time_point<steady_clock> now) noexcept {
	if (_lastSecondTime == time_point<steady_clock>{}) {
		// 第一帧
		_lastSecondTime = now;
		_framesPerSecond.store(1, std::memory_order_relaxed);
		return;
	}

	const nanoseconds delta = now - _lastSecondTime;
	if (delta >= 1s) {
		_lastSecondTime = now - delta % 1s;

		_framesPerSecond.store(_framesThisSecond, std::memory_order_relaxed);
		_framesThisSecond = 0;
	}
}

bool StepTimer::_HasMinInterval() const noexcept {
	return _minInterval.count() != 0;
}

bool StepTimer::_HasMaxInterval() const noexcept {
	return _maxInterval.count() != std::numeric_limits<nanoseconds::rep>::max();
}

}
