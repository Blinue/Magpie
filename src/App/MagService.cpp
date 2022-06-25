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
	_isAutoRestoreChangedRevoker = _settings.IsAutoRestoreChanged(
		auto_revoke,
		{this, &MagService::_Settings_IsAutoRestoreChanged }
	);

	_UpdateIsAutoRestore();
}

void MagService::ClearWndToRestore() {
	_wndToRestore = 0;
	_wndToRestoreChangedEvent(*this, _wndToRestore);
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
		_hEventHook = SetWinEventHook(
			EVENT_SYSTEM_FOREGROUND,
			EVENT_SYSTEM_FOREGROUND,
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
		if (_hEventHook) {
			UnhookWinEvent(_hEventHook);
			_hEventHook = NULL;
		}
	}
}

void MagService::_CheckForeground() {
	if (_magRuntime.IsRunning() || (HWND)_wndToRestore != GetForegroundWindow()) {
		return;
	}

	_magRuntime.Run(_wndToRestore, Magpie::Runtime::MagSettings());
}

void MagService::_WinEventProcCallback(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD) {
	_that->_CheckForeground();
}

}
