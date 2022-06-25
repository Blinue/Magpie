#include "pch.h"
#include "MagService.h"
#if __has_include("MagService.g.cpp")
#include "MagService.g.cpp"
#endif

namespace winrt::Magpie::App::implementation {

MagService::MagService(
	Magpie::App::Settings const& settings,
	Magpie::Runtime::MagRuntime const& magRuntime,
	CoreDispatcher const& dispatcher
) : _settings(settings), _magRuntime(magRuntime) {
	if (_settings.IsAutoRestore()) {
		_isRunningChangedRevoker = magRuntime.IsRunningChanged(
			auto_revoke,
			[this, dispatcher](IInspectable const&, bool) -> IAsyncAction {
				co_await dispatcher.RunAsync(CoreDispatcherPriority::Normal, [this]() {
					if (_magRuntime.IsRunning()) {
						_curSrcWnd = (HWND)_magRuntime.HwndSrc();
						_wndToRestore = 0;
						_wndToRestoreChangedEvent(*this, _wndToRestore);
					} else {
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
}

}
