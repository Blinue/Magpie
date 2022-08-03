#include "pch.h"
#include "MagService.h"
#include "HotkeyService.h"
#include "Win32Utils.h"
#include "AppSettings.h"
#include "ScalingProfileService.h"


namespace winrt::Magpie::App {

void MagService::Initialize() {
	_dispatcher = CoreWindow::GetForCurrentThread().Dispatcher();

	_timer.Interval(TimeSpan(std::chrono::milliseconds(25)));
	_timerTickRevoker = _timer.Tick(
		auto_revoke,
		{ this, &MagService::_Timer_Tick }
	);

	AppSettings::Get().IsAutoRestoreChanged({ this, &MagService::_Settings_IsAutoRestoreChanged });
	_magRuntime.IsRunningChanged({ this, &MagService::_MagRuntime_IsRunningChanged });

	HotkeyService::Get().HotkeyPressed(
		{ this, &MagService::_HotkeyService_HotkeyPressed }
	);

	_UpdateIsAutoRestore();
}

void MagService::StartCountdown() {
	if (_tickingDownCount != 0) {
		return;
	}

	_tickingDownCount = AppSettings::Get().DownCount();
	_timerStartTimePoint = std::chrono::steady_clock::now();
	_timer.Start();
	_isCountingDownChangedEvent(true);
}

void MagService::StopCountdown() {
	if (_tickingDownCount == 0) {
		return;
	}

	_tickingDownCount = 0;
	_timer.Stop();
	_isCountingDownChangedEvent(false);
}

float MagService::CountdownLeft() const noexcept {
	using namespace std::chrono;

	if (!IsCountingDown()) {
		return 0.0f;
	}

	// DispatcherTimer 误差很大，因此我们自己计算剩余时间
	auto now = steady_clock::now();
	int timeLeft = (int)duration_cast<milliseconds>(_timerStartTimePoint + seconds(_tickingDownCount) - now).count();
	return timeLeft / 1000.0f;
}

void MagService::ClearWndToRestore() {
	if (_wndToRestore == 0) {
		return;
	}

	_wndToRestore = 0;
	_wndToRestoreChangedEvent(_wndToRestore);
}

void MagService::_HotkeyService_HotkeyPressed(HotkeyAction action) {
	switch (action) {
	case HotkeyAction::Scale:
	{
		if (_magRuntime.IsRunning()) {
			_magRuntime.Stop();
			return;
		}

		_StartScale();
		break;
	}
	case HotkeyAction::Overlay:
	{
		if (_magRuntime.IsRunning()) {
			_magRuntime.ToggleOverlay();
			return;
		}
		break;
	}
	default:
		break;
	}
}

void MagService::_Timer_Tick(IInspectable const&, IInspectable const&) {
	float timeLeft = CountdownLeft();

	// 剩余时间在 10 ms 以内计时结束
	if (timeLeft < 0.01) {
		StopCountdown();
		_StartScale();
		return;
	}

	_countdownTickEvent(timeLeft);
}

void MagService::_Settings_IsAutoRestoreChanged(bool) {
	_UpdateIsAutoRestore();
}

IAsyncAction MagService::_MagRuntime_IsRunningChanged(IInspectable const&, bool) {
	co_await _dispatcher.RunAsync(CoreDispatcherPriority::Normal, [this]() {
		bool isRunning = _magRuntime.IsRunning();
		if (isRunning) {
			StopCountdown();

			if (AppSettings::Get().IsAutoRestore()) {
				_curSrcWnd = (HWND)_magRuntime.HwndSrc();
				_wndToRestore = 0;
				_wndToRestoreChangedEvent(_wndToRestore);
			}
		} else {
			HWND hwndMain = (HWND)Application::Current().as<Magpie::App::App>().HwndMain();

			// 必须在主线程还原主窗口样式
			// 见 FrameSourceBase::~FrameSourceBase
			LONG_PTR style = GetWindowLongPtr(hwndMain, GWL_STYLE);
			if (!(style & WS_THICKFRAME)) {
				SetWindowLongPtr(hwndMain, GWL_STYLE, style | WS_THICKFRAME);
				SetWindowPos(hwndMain, 0, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}

			if (AppSettings::Get().IsAutoRestore()) {
				// 退出全屏之后前台窗口不变则不必记忆
				if (IsWindow(_curSrcWnd) && GetForegroundWindow() != _curSrcWnd) {
					_wndToRestore = (uint64_t)_curSrcWnd;
					_wndToRestoreChangedEvent(_wndToRestore);
				}

				_curSrcWnd = NULL;
			}
		}

		_isRunningChangedEvent(isRunning);
	});
}

void MagService::_UpdateIsAutoRestore() {
	if (AppSettings::Get().IsAutoRestore()) {
		// 立即生效，即使正处于缩放状态
		_curSrcWnd = (HWND)_magRuntime.HwndSrc();

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
		_curSrcWnd = NULL;
		_wndToRestore = 0;
		_wndToRestoreChangedEvent(_wndToRestore);
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
	if (_wndToRestore == 0 || _magRuntime.IsRunning()) {
		return;
	}

	if (!IsWindow((HWND)_wndToRestore)) {
		_wndToRestore = 0;
		_wndToRestoreChangedEvent(_wndToRestore);
		return;
	}

	if ((HWND)_wndToRestore != GetForegroundWindow()) {
		return;
	}

	_StartScale(_wndToRestore);
}

void MagService::_StartScale(uint64_t hWnd) {
	if (hWnd == 0) {
		hWnd = (uint64_t)GetForegroundWindow();
	}

	if (Win32Utils::GetWindowShowCmd((HWND)hWnd) != SW_NORMAL) {
		return;
	}

	const ScalingProfile& profile = ScalingProfileService::Get().GetProfileForWindow((HWND)hWnd);
	
	Magpie::Runtime::MagSettings magSettings;
	magSettings.CopyFrom(profile.MagSettings());

	if (!profile.IsCroppingEnabled()) {
		magSettings.Cropping({});
	}

	double cursorScaling;
	switch (profile.CursorScaling()) {
	case CursorScaling::x0_5:
		cursorScaling = 0.5;
		break;
	case CursorScaling::x0_75:
		cursorScaling = 0.75;
		break;
	case CursorScaling::NoScaling:
		cursorScaling = 1.0;
		break;
	case CursorScaling::x1_25:
		cursorScaling = 1.25;
		break;
	case CursorScaling::x1_5:
		cursorScaling = 1.5;
		break;
	case CursorScaling::x2:
		cursorScaling = 2.0;
		break;
	case CursorScaling::Source:
		// 0 或负值表示和源窗口缩放比例相同
		cursorScaling = 0;
		break;
	case CursorScaling::Custom:
		cursorScaling = profile.CustomCursorScaling();
		break;
	default:
		cursorScaling = 1.0;
		break;
	}
	magSettings.CursorScaling(cursorScaling);

	// 应用全局配置
	AppSettings& settings = AppSettings::Get();
	magSettings.IsBreakpointMode(settings.IsBreakpointMode());
	magSettings.IsDisableEffectCache(settings.IsDisableEffectCache());
	magSettings.IsSaveEffectSources(settings.IsSaveEffectSources());
	magSettings.IsWarningsAreErrors(settings.IsWarningsAreErrors());
	magSettings.IsSimulateExclusiveFullscreen(settings.IsSimulateExclusiveFullscreen());

	_magRuntime.Run(hWnd, magSettings);
}

void MagService::_WinEventProcCallback(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD) {
	MagService::Get()._CheckForeground();
}

}