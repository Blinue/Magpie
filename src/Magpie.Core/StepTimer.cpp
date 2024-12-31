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

// 渲染新帧时以该次循环开始捕获的时间点作为新帧的开始时间，只有这个时间点是我们可以控制的。
// 下图中的 wait 包括等待最大帧率限制和等待新帧，WaitForNextFrame 在此期间多次执行。
// PrepareForRender 在 render 开始前执行。
// 
// _thisFrameStartTime      _nextFrameStartTime
//         │                         │
// ────────▼─────────┬────────┬──────▼─────────
//    wait │ capture │ render │ wait │ capture
//
StepTimerStatus StepTimer::WaitForNextFrame(bool waitMsgForNewFrame) noexcept {
	// 不断更新 _nextFrameStartTime 直到新帧到达
	_nextFrameStartTime = steady_clock::now();

	if (_thisFrameStartTime == time_point<steady_clock>{}) {
		// 等待第一帧，无需更新 FPS
		if (waitMsgForNewFrame) {
			WaitMessage();
		}

		return StepTimerStatus::WaitForNewFrame;
	}

	// 包括当前帧的捕获时间和渲染时间以及渲染完成后已经等待的时间
	const nanoseconds delta = _nextFrameStartTime - _thisFrameStartTime;

	if (delta >= _maxInterval) {
		return StepTimerStatus::ForceNewFrame;
	}

	// 没有新帧也应更新 FPS。作为性能优化，强制帧无需更新，因为 PrepareForRender 必定会执行
	_UpdateFPS(_nextFrameStartTime);

	if (delta < _minInterval) {
		_WaitForMsgAndTimer(_minInterval - delta);
		return StepTimerStatus::WaitForFPSLimiter;
	}

	// 有的捕获方法当有新帧时会有消息到达
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

void StepTimer::PrepareForRender() noexcept {
	// 进入新一帧，计算此帧的开始时间
	if (_HasMinInterval()) {
		// 限制最大帧率时帧间隔必须是最小帧间隔的整数倍，_nextFrameStartTime 需要稍微向前修正。
		// 出于同样的原因，最大帧间隔应是最小帧间隔的整数倍。
		_thisFrameStartTime = _nextFrameStartTime -
			(_nextFrameStartTime - _thisFrameStartTime) % _minInterval;
	} else {
		_thisFrameStartTime = _nextFrameStartTime;
	}

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
	} else {
		const nanoseconds delta = now - _lastSecondTime;
		if (delta < 1s) {
			return;
		}

		_lastSecondTime = now - delta % 1s;
	}

	_framesPerSecond.store(_framesThisSecond, std::memory_order_relaxed);
	_framesThisSecond = 0;
}

bool StepTimer::_HasMinInterval() const noexcept {
	return _minInterval.count() != 0;
}

bool StepTimer::_HasMaxInterval() const noexcept {
	return _maxInterval.count() != std::numeric_limits<nanoseconds::rep>::max();
}

}
