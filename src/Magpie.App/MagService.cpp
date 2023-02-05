#include "pch.h"
#include "MagService.h"
#include "HotkeyService.h"
#include "Win32Utils.h"
#include "AppSettings.h"
#include "ScalingProfileService.h"
#include "ScalingModesService.h"
#include "ScalingMode.h"
#include "Logger.h"

using namespace ::Magpie::Core;
using namespace winrt;
using namespace Windows::System::Threading;

namespace winrt::Magpie::App {

void MagService::Initialize() {
	_dispatcher = CoreWindow::GetForCurrentThread().Dispatcher();
	
	_countDownTimer.Interval(25ms);
	_countDownTimer.Tick({ this, &MagService::_CountDownTimer_Tick });

	_checkForegroundTimer = ThreadPoolTimer::CreatePeriodicTimer(
		{ this, &MagService::_CheckForegroundTimer_Tick },
		50ms
	);
	
	AppSettings::Get().IsAutoRestoreChanged({ this, &MagService::_Settings_IsAutoRestoreChanged });
	_magRuntime.IsRunningChanged({ this, &MagService::_MagRuntime_IsRunningChanged });

	HotkeyService::Get().HotkeyPressed(
		{ this, &MagService::_HotkeyService_HotkeyPressed }
	);

	// 立即检查前台窗口
	_CheckForegroundTimer_Tick(nullptr);
}

void MagService::StartCountdown() {
	if (_tickingDownCount != 0) {
		return;
	}

	_tickingDownCount = AppSettings::Get().DownCount();
	_timerStartTimePoint = std::chrono::steady_clock::now();
	_countDownTimer.Start();
	_isCountingDownChangedEvent(true);
}

void MagService::StopCountdown() {
	if (_tickingDownCount == 0) {
		return;
	}

	_tickingDownCount = 0;
	_countDownTimer.Stop();
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

void MagService::CheckForeground() {
	_hwndChecked = NULL;
	_CheckForegroundTimer_Tick(nullptr);
}

void MagService::_WndToRestore(HWND value) {
	if (_hwndToRestore == value) {
		return;
	}

	_hwndToRestore = value;
	_wndToRestoreChangedEvent(_hwndToRestore);
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

void MagService::_CountDownTimer_Tick(IInspectable const&, IInspectable const&) {
	float timeLeft = CountdownLeft();

	// 剩余时间在 10 ms 以内计时结束
	if (timeLeft < 0.01) {
		StopCountdown();
		_ScaleForegroundWindow();
		return;
	}

	_countdownTickEvent(timeLeft);
}

fire_and_forget MagService::_CheckForegroundTimer_Tick(ThreadPoolTimer const& timer) {
	if (_magRuntime.IsRunning()) {
		co_return;
	}

	HWND hwndFore = GetForegroundWindow();
	if (hwndFore == _hwndChecked) {
		co_return;
	}
	_hwndChecked = NULL;

	if (timer) {
		// ThreadPoolTimer 在后台线程触发
		co_await _dispatcher;
	}
	
	const bool isAutoRestore = AppSettings::Get().IsAutoRestore();

	if (_CheckSrcWnd(hwndFore)) {
		const ScalingProfile& profile = ScalingProfileService::Get().GetProfileForWindow(hwndFore);
		// 先检查自动恢复全屏
		if (profile.isAutoScale) {
			if (_StartScale(hwndFore, profile)) {
				// 触发自动缩放时清空记忆的窗口
				if (AppSettings::Get().IsAutoRestore()) {
					_WndToRestore(NULL);
				}
			}

			co_return;
		}

		// 恢复记忆的窗口
		if (isAutoRestore && _hwndToRestore == hwndFore) {
			_StartScale(hwndFore, profile);
			co_return;
		}
	}

	if (isAutoRestore && !_CheckSrcWnd(_hwndToRestore)) {
		_WndToRestore(NULL);
	}

	// 避免重复检查
	_hwndChecked = hwndFore;
}

void MagService::_Settings_IsAutoRestoreChanged(bool) {
	if (AppSettings::Get().IsAutoRestore()) {
		// 立即生效，即使正处于缩放状态
		_hwndCurSrc = _magRuntime.HwndSrc();
	} else {
		_hwndCurSrc = NULL;
		_WndToRestore(NULL);
	}
}

fire_and_forget MagService::_MagRuntime_IsRunningChanged(bool isRunning) {
	co_await _dispatcher;

	if (isRunning) {
		StopCountdown();

		if (AppSettings::Get().IsAutoRestore()) {
			_WndToRestore(NULL);
		}

		_hwndCurSrc = _magRuntime.HwndSrc();
	} else {
		HWND curSrcWnd = _hwndCurSrc;
		_hwndCurSrc = NULL;

		HWND hwndMain = (HWND)Application::Current().as<App>().HwndMain();
		if (hwndMain == curSrcWnd) {
			// 必须在主线程还原主窗口样式
			// 见 FrameSourceBase::~FrameSourceBase
			LONG_PTR style = GetWindowLongPtr(hwndMain, GWL_STYLE);
			if (!(style & WS_THICKFRAME)) {
				SetWindowLongPtr(hwndMain, GWL_STYLE, style | WS_THICKFRAME);
				SetWindowPos(hwndMain, 0, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}
		}

		if (GetForegroundWindow() == curSrcWnd) {
			// 退出全屏后如果前台窗口不变视为通过热键退出
			_hwndChecked = curSrcWnd;
		} else if (!_isAutoScaling && AppSettings::Get().IsAutoRestore()) {
			if (_CheckSrcWnd(curSrcWnd)) {
				_WndToRestore(curSrcWnd);
			}
		}

		// 立即检查前台窗口
		_CheckForegroundTimer_Tick(nullptr);
	}

	_isRunningChangedEvent(isRunning);
}

bool MagService::_StartScale(HWND hWnd, const ScalingProfile& profile) {
	if (profile.scalingMode < 0) {
		return false;
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

	_isAutoScaling = profile.isAutoScale;
	_magRuntime.Run(hWnd, options);
	return true;
}

void MagService::_ScaleForegroundWindow() {
	HWND hWnd = GetForegroundWindow();
	if (!_CheckSrcWnd(hWnd)) {
		return;
	}

	const ScalingProfile& profile = ScalingProfileService::Get().GetProfileForWindow((HWND)hWnd);
	_StartScale(hWnd, profile);
}

bool MagService::_CheckSrcWnd(HWND hWnd) noexcept {
	return hWnd && Win32Utils::GetWindowShowCmd(hWnd) == SW_NORMAL;
}

}
