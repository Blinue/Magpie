#include "pch.h"
#include "MagService.h"
#include "HotkeyService.h"
#include "Win32Utils.h"
#include "AppSettings.h"
#include "ScalingProfileService.h"
#include "ScalingModesService.h"
#include "ScalingMode.h"
#include "Logger.h"

using namespace Magpie::Core;

namespace winrt::Magpie::App {

MagService::~MagService() {
	if (_hForegroundEventHook) {
		UnhookWinEvent(_hForegroundEventHook);
	}
	if (_hDestoryEventHook) {
		UnhookWinEvent(_hDestoryEventHook);
	}
}

void MagService::Initialize() {
	_dispatcher = CoreWindow::GetForCurrentThread().Dispatcher();
	
	_timer.Interval(25ms);
	_timer.Tick({ this, &MagService::_Timer_Tick });

	AppSettings::Get().IsAutoRestoreChanged({ this, &MagService::_Settings_IsAutoRestoreChanged });
	_magRuntime.IsRunningChanged({ this, &MagService::_MagRuntime_IsRunningChanged });

	HotkeyService::Get().HotkeyPressed(
		{ this, &MagService::_HotkeyService_HotkeyPressed }
	);

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

	if (!_hForegroundEventHook || !_hDestoryEventHook) {
		assert(false);
		Logger::Get().Win32Error("监听前台窗口更改失败");
	}

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
	_WndToRestore(NULL);
}

void MagService::_WndToRestore(HWND value) {
	if (_wndToRestore == value) {
		return;
	}

	_wndToRestore = value;
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

		_ScaleForegroundWindow();
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
		_ScaleForegroundWindow();
		return;
	}

	_countdownTickEvent(timeLeft);
}

void MagService::_Settings_IsAutoRestoreChanged(bool) {
	_UpdateIsAutoRestore();
}

fire_and_forget MagService::_MagRuntime_IsRunningChanged(bool isRunning) {
	co_await _dispatcher;

	if (isRunning) {
		StopCountdown();

		if (AppSettings::Get().IsAutoRestore()) {
			_WndToRestore(NULL);
		}

		_curSrcWnd = _magRuntime.HwndSrc();
	} else {
		HWND hwndMain = (HWND)Application::Current().as<App>().HwndMain();
		if (hwndMain == _curSrcWnd) {
			// 必须在主线程还原主窗口样式
			// 见 FrameSourceBase::~FrameSourceBase
			LONG_PTR style = GetWindowLongPtr(hwndMain, GWL_STYLE);
			if (!(style & WS_THICKFRAME)) {
				SetWindowLongPtr(hwndMain, GWL_STYLE, style | WS_THICKFRAME);
				SetWindowPos(hwndMain, 0, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}
		}
		
		HWND hwndFore = GetForegroundWindow();
		if (hwndFore == _curSrcWnd) {
			// 退出全屏后前台窗口不变
			_curSrcWnd = NULL;
			co_return;
		}

		// 检查自动缩放
		if (_CheckSrcWnd(hwndFore)) {
			ScalingProfile& profile = ScalingProfileService::Get().GetProfileForWindow(hwndFore);
			if (profile.isAutoScale) {
				_curSrcWnd = NULL;
				_StartScale(hwndFore, profile, true);
				co_return;
			}
		}

		if (!_isAutoScaling && AppSettings::Get().IsAutoRestore()) {
			if (_CheckSrcWnd(_curSrcWnd)) {
				_WndToRestore(_curSrcWnd);
			}
		}

		_curSrcWnd = NULL;
	}

	_isRunningChangedEvent(isRunning);
}

void MagService::_UpdateIsAutoRestore() {
	if (AppSettings::Get().IsAutoRestore()) {
		// 立即生效，即使正处于缩放状态
		_curSrcWnd = _magRuntime.HwndSrc();
	} else {
		_curSrcWnd = NULL;
		_WndToRestore(NULL);
	}
}

void MagService::_CheckForeground() {
	if (_magRuntime.IsRunning()) {
		return;
	}

	HWND hwndForeground = GetForegroundWindow();
	const ScalingProfile* profile = nullptr;
	if (_CheckSrcWnd(hwndForeground)) {
		profile = &ScalingProfileService::Get().GetProfileForWindow(hwndForeground);
		if (profile->isAutoScale) {
			_StartScale(hwndForeground, *profile, true);
			
			// 触发自动缩放时清空记忆的窗口
			if (AppSettings::Get().IsAutoRestore()) {
				_WndToRestore(NULL);
			}

			return;
		}
	}

	if (!AppSettings::Get().IsAutoRestore() || !_wndToRestore) {
		return;
	}

	if (!IsWindow(_wndToRestore)) {
		_WndToRestore(NULL);
		return;
	}

	if (_wndToRestore != hwndForeground) {
		return;
	}

	if (!profile) {
		profile = &ScalingProfileService::Get().GetProfileForWindow(hwndForeground);
	}
	_StartScale(hwndForeground, *profile, false);
}

void MagService::_StartScale(HWND hWnd, const ScalingProfile& profile, bool isAutoScale) {
	if (profile.scalingMode < 0) {
		return;
	}

	MagOptions options;
	options.graphicsAdapter = profile.graphicsAdapter;
	options.captureMethod = profile.captureMethod;
	options.multiMonitorUsage = profile.multiMonitorUsage;
	options.cursorInterpolationMode = profile.cursorInterpolationMode;
	options.flags = profile.flags;

	if (profile.isCroppingEnabled) {
		options.cropping = profile.cropping;
	}

	switch (profile.cursorScaling) {
	case CursorScaling::x0_5:
		options.cursorScaling = 0.5;
		break;
	case CursorScaling::x0_75:
		options.cursorScaling = 0.75;
		break;
	case CursorScaling::NoScaling:
		options.cursorScaling = 1.0;
		break;
	case CursorScaling::x1_25:
		options.cursorScaling = 1.25;
		break;
	case CursorScaling::x1_5:
		options.cursorScaling = 1.5;
		break;
	case CursorScaling::x2:
		options.cursorScaling = 2.0;
		break;
	case CursorScaling::Source:
		// 0 或负值表示和源窗口缩放比例相同
		options.cursorScaling = 0;
		break;
	case CursorScaling::Custom:
		options.cursorScaling = profile.customCursorScaling;
		break;
	default:
		options.cursorScaling = 1.0;
		break;
	}

	AppSettings& settings = AppSettings::Get();

	options.downscalingEffect = settings.DownscalingEffect();

	// 应用缩放模式
	options.effects = ScalingModesService::Get().GetScalingMode(profile.scalingMode).effects;
	if (settings.IsInlineParams()) {
		for (EffectOption& effect : options.effects) {
			effect.flags |= EffectOptionFlags::InlineParams;
		}
	}

	// 应用全局配置
	options.IsDebugMode(settings.IsDebugMode());
	options.IsDisableEffectCache(settings.IsDisableEffectCache());
	options.IsSaveEffectSources(settings.IsSaveEffectSources());
	options.IsWarningsAreErrors(settings.IsWarningsAreErrors());
	options.IsSimulateExclusiveFullscreen(settings.IsSimulateExclusiveFullscreen());

	_isAutoScaling = isAutoScale;
	_magRuntime.Run(hWnd, options);
}

void MagService::_ScaleForegroundWindow() {
	HWND hWnd = GetForegroundWindow();
	if (!_CheckSrcWnd(hWnd)) {
		return;
	}

	const ScalingProfile& profile = ScalingProfileService::Get().GetProfileForWindow((HWND)hWnd);
	_StartScale(hWnd, profile, false);
}

bool MagService::_CheckSrcWnd(HWND hWnd) noexcept {
	return hWnd && Win32Utils::GetWindowShowCmd(hWnd) == SW_NORMAL;
}

void MagService::_WinEventProcCallback(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD) {
	MagService::Get()._CheckForeground();
}

}
