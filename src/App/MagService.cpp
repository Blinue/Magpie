#include "pch.h"
#include "MagService.h"
#if __has_include("MagService.g.cpp")
#include "MagService.g.cpp"
#endif


namespace winrt::Magpie::App::implementation {

MagService* MagService::_that = nullptr;

MagService::MagService(
	Magpie::App::Settings const& settings,
	Magpie::Runtime::MagRuntime const& magRuntime,
	CoreDispatcher const& dispatcher
) : _settings(settings), _magRuntime(magRuntime), _dispatcher(dispatcher) {
	_timer.Interval(TimeSpan(std::chrono::milliseconds(25)));
	_timerTickRevoker = _timer.Tick(
		auto_revoke,
		{ this, &MagService::_Timer_Tick }
	);

	_isAutoRestoreChangedRevoker = _settings.IsAutoRestoreChanged(
		auto_revoke,
		{ this, &MagService::_Settings_IsAutoRestoreChanged }
	);

	_UpdateIsAutoRestore();
}

void MagService::StartCountdown() {
	if (_tickingDownCount != 0) {
		return;
	}

	_tickingDownCount = _settings.DownCount();
	_timerStartTimePoint = std::chrono::steady_clock::now();
	_timer.Start();
	_isCountingDownChangedEvent(*this, true);
}

void MagService::StopCountdown() {
	if (_tickingDownCount == 0) {
		return;
	}

	_tickingDownCount = 0;
	_timer.Stop();
	_isCountingDownChangedEvent(*this, false);
}

void MagService::ClearWndToRestore() {
	if (_wndToRestore == 0) {
		return;
	}

	_wndToRestore = 0;
	_wndToRestoreChangedEvent(*this, _wndToRestore);
}

void MagService::_Timer_Tick(IInspectable const&, IInspectable const&) {
	using namespace std::chrono;

	// DispatcherTimer 误差很大，因此我们自己计算剩余时间
	auto now = steady_clock::now();
	int timeLeft = (int)duration_cast<milliseconds>(_timerStartTimePoint + seconds(_tickingDownCount) - now).count();

	// 剩余时间在 10 ms 以内计时结束
	if (timeLeft < 10) {
		StopCountdown();
		_magRuntime.Run((uint64_t)GetForegroundWindow(), Magpie::Runtime::MagSettings());
		return;
	}
	
	_countdownTickEvent(*this, timeLeft / 1000.0f);
}

void MagService::_Settings_IsAutoRestoreChanged(IInspectable const&, bool) {
	_UpdateIsAutoRestore();
}

void MagService::_UpdateIsAutoRestore() {
	if (_settings.IsAutoRestore()) {
		if (!_isRunningChangedRevoker) {
			_isRunningChangedRevoker = _magRuntime.IsRunningChanged(
				auto_revoke,
				[this](IInspectable const&, bool) -> IAsyncAction {
					co_await _dispatcher.RunAsync(CoreDispatcherPriority::Normal, [this]() {
						if (_magRuntime.IsRunning()) {
							_curSrcWnd = (HWND)_magRuntime.HwndSrc();
							_wndToRestore = 0;
							_wndToRestoreChangedEvent(*this, _wndToRestore);

							StopCountdown();
						} else {
							// 退出全屏之后前台窗口不变则不必记忆
							if (IsWindow(_curSrcWnd) && GetForegroundWindow() != _curSrcWnd) {
								_wndToRestore = (uint64_t)_curSrcWnd;
								_wndToRestoreChangedEvent(*this, _wndToRestore);
							}

							_curSrcWnd = NULL;
						}
					});
				}
			);
		}
		
		// 立即生效，即使正处于缩放状态
		_curSrcWnd = (HWND)_magRuntime.HwndSrc();

		_that = this;
		// 监听前台窗口更改
		_hForegroundEventHook = SetWinEventHook(
			EVENT_SYSTEM_FOREGROUND,
			EVENT_SYSTEM_FOREGROUND,
			NULL,
			_WinEventProcCallback,
			0,
			0,
			WINEVENT_OUTOFCONTEXT
		);
		// 监听窗口销毁
		_hDestoryEventHook = SetWinEventHook(
			EVENT_OBJECT_DESTROY,
			EVENT_OBJECT_DESTROY,
			NULL,
			_WinEventProcCallback,
			0,
			0,
			WINEVENT_OUTOFCONTEXT
		);
	} else {
		_isRunningChangedRevoker = {};
		_curSrcWnd = NULL;
		_wndToRestore = 0;
		_wndToRestoreChangedEvent(*this, _wndToRestore);
		if (_hForegroundEventHook) {
			UnhookWinEvent(_hForegroundEventHook);
			_hForegroundEventHook = NULL;
		}
		if (_hDestoryEventHook) {
			UnhookWinEvent(_hDestoryEventHook);
			_hDestoryEventHook = NULL;
		}
	}
}

void MagService::_CheckForeground() {
	if (!_wndToRestore || _magRuntime.IsRunning()) {
		return;
	}

	if (!IsWindow((HWND)_wndToRestore)) {
		_wndToRestore = 0;
		_wndToRestoreChangedEvent(*this, _wndToRestore);
		return;
	}

	if ((HWND)_wndToRestore != GetForegroundWindow()) {
		return;
	}

	_magRuntime.Run(_wndToRestore, Magpie::Runtime::MagSettings());
}

void MagService::_WinEventProcCallback(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD) {
	_that->_CheckForeground();
}

}
