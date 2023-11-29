#include "pch.h"
#include "MagService.h"
#include "ShortcutService.h"
#include "Win32Utils.h"
#include "AppSettings.h"
#include "ProfileService.h"
#include "ScalingModesService.h"
#include "ScalingMode.h"
#include "Logger.h"
#include "EffectsService.h"

using namespace ::Magpie::Core;
using namespace winrt;
using namespace Windows::System::Threading;

namespace winrt::Magpie::App {

void MagService::Initialize() {
	_dispatcher = CoreWindow::GetForCurrentThread().Dispatcher();
	
	_countDownTimer.Interval(25ms);
	_countDownTimer.Tick({ this, &MagService::_CountDownTimer_Tick });

	AppSettings::Get().IsAutoRestoreChanged({ this, &MagService::_Settings_IsAutoRestoreChanged });
	_magRuntime.emplace();
	_magRuntime->IsRunningChanged({ this, &MagService::_MagRuntime_IsRunningChanged });

	ShortcutService::Get().ShortcutActivated(
		{ this, &MagService::_ShortcutService_ShortcutPressed }
	);

	_checkForegroundTimer = ThreadPoolTimer::CreatePeriodicTimer(
		{ this, &MagService::_CheckForegroundTimer_Tick },
		50ms
	);

	// 立即检查前台窗口
	_CheckForegroundTimer_Tick(nullptr);
}

void MagService::Uninitialize() {
	_checkForegroundTimer.Cancel();
	_countDownTimer.Stop();
	_magRuntime.reset();
}

void MagService::StartTimer() {
	if (_curCountdownSeconds != 0) {
		return;
	}

	_curCountdownSeconds = AppSettings::Get().CountdownSeconds();
	_timerStartTimePoint = std::chrono::steady_clock::now();
	_countDownTimer.Start();
	_isTimerOnChangedEvent(true);
}

void MagService::StopTimer() {
	if (_curCountdownSeconds == 0) {
		return;
	}

	_curCountdownSeconds = 0;
	_countDownTimer.Stop();
	_isTimerOnChangedEvent(false);
}

double MagService::SecondsLeft() const noexcept {
	using namespace std::chrono;

	if (!IsTimerOn()) {
		return 0.0;
	}

	// DispatcherTimer 误差很大，因此我们自己计算剩余时间
	auto now = steady_clock::now();
	int msLeft = (int)duration_cast<milliseconds>(_timerStartTimePoint + seconds(_curCountdownSeconds) - now).count();
	return msLeft / 1000.0;
}

void MagService::ClearWndToRestore() {
	_WndToRestore(NULL);
}

void MagService::CheckForeground() {
	_hwndChecked.store(NULL, std::memory_order_relaxed);
	_CheckForegroundTimer_Tick(nullptr);
}

void MagService::_WndToRestore(HWND value) {
	if (_hwndToRestore == value) {
		return;
	}

	_hwndToRestore = value;
	_wndToRestoreChangedEvent(_hwndToRestore);
}

void MagService::_ShortcutService_ShortcutPressed(ShortcutAction action) {
	if (!_magRuntime) {
		return;
	}

	switch (action) {
	case ShortcutAction::Scale:
	{
		if (_magRuntime->IsRunning()) {
			_magRuntime->Stop();
			return;
		}

		_ScaleForegroundWindow();
		break;
	}
	case ShortcutAction::Overlay:
	{
		if (_magRuntime->IsRunning()) {
			_magRuntime->ToggleOverlay();
			return;
		}
		break;
	}
	default:
		break;
	}
}

void MagService::_CountDownTimer_Tick(IInspectable const&, IInspectable const&) {
	double timeLeft = SecondsLeft();

	// 剩余时间在 10 ms 以内计时结束
	if (timeLeft < 0.01) {
		StopTimer();
		_ScaleForegroundWindow();
		return;
	}

	_timerTickEvent(timeLeft);
}

fire_and_forget MagService::_CheckForegroundTimer_Tick(ThreadPoolTimer const& timer) {
	// _magRuntime 不会改变，无需同步措施
	if (!_magRuntime || _magRuntime->IsRunning()) {
		co_return;
	}

	const HWND hwndFore = GetForegroundWindow();
	// 没有对其他变量的访问，因此不需要限制内存定序
	if (hwndFore == _hwndChecked.load(std::memory_order_relaxed)) {
		co_return;
	}
	_hwndChecked.store(NULL, std::memory_order_relaxed);

	if (timer) {
		// ThreadPoolTimer 在后台线程触发
		co_await _dispatcher;
	}
	
	const bool isAutoRestore = AppSettings::Get().IsAutoRestore();

	if (_CheckSrcWnd(hwndFore)) {
		const Profile& profile = ProfileService::Get().GetProfileForWindow(hwndFore);
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
	_hwndChecked.store(hwndFore, std::memory_order_relaxed);
}

void MagService::_Settings_IsAutoRestoreChanged(bool) {
	if (AppSettings::Get().IsAutoRestore()) {
		// 立即生效，即使正处于缩放状态
		_hwndCurSrc = _magRuntime->HwndSrc();
	} else {
		_hwndCurSrc = NULL;
		_WndToRestore(NULL);
	}
}

fire_and_forget MagService::_MagRuntime_IsRunningChanged(bool isRunning) {
	co_await _dispatcher;

	if (isRunning) {
		StopTimer();

		if (AppSettings::Get().IsAutoRestore()) {
			_WndToRestore(NULL);
		}

		_hwndCurSrc = _magRuntime->HwndSrc();
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
			_hwndChecked.store(curSrcWnd, std::memory_order_relaxed);
		} else if (!_isAutoScaling && AppSettings::Get().IsAutoRestore()) {
			// 自动缩放窗口时切换窗口无需记录到自动恢复
			if (_CheckSrcWnd(curSrcWnd)) {
				_WndToRestore(curSrcWnd);
			}
		}

		// 立即检查前台窗口
		_CheckForegroundTimer_Tick(nullptr);
	}

	_isRunningChangedEvent(isRunning);
}

bool MagService::_StartScale(HWND hWnd, const Profile& profile) {
	if (profile.scalingMode < 0) {
		return false;
	}

	MagOptions options;
	options.effects = ScalingModesService::Get().GetScalingMode(profile.scalingMode).effects;
	if (options.effects.empty()) {
		return false;
	} else {
		for (EffectOption& effect : options.effects) {
			if (!EffectsService::Get().GetEffect(effect.name)) {
				// 存在无法解析的效果
				return false;
			}
		}
	}
	
	options.graphicsCard = profile.graphicsCard;
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

	// 应用全局配置
	AppSettings& settings = AppSettings::Get();

	if (settings.IsInlineParams()) {
		for (EffectOption& effect : options.effects) {
			effect.flags |= EffectOptionFlags::InlineParams;
		}
	}

	options.downscalingEffect = settings.DownscalingEffect();
	options.IsDebugMode(settings.IsDebugMode());
	options.IsDisableEffectCache(settings.IsDisableEffectCache());
	options.IsDisableFontCache(settings.IsDisableFontCache());
	options.IsSaveEffectSources(settings.IsSaveEffectSources());
	options.IsWarningsAreErrors(settings.IsWarningsAreErrors());
	options.IsAllowScalingMaximized(settings.IsAllowScalingMaximized());
	options.IsSimulateExclusiveFullscreen(settings.IsSimulateExclusiveFullscreen());

	_isAutoScaling = profile.isAutoScale;
	_magRuntime->Run(hWnd, options);
	return true;
}

void MagService::_ScaleForegroundWindow() {
	HWND hWnd = GetForegroundWindow();
	if (!_CheckSrcWnd(hWnd)) {
		return;
	}

	const Profile& profile = ProfileService::Get().GetProfileForWindow((HWND)hWnd);
	_StartScale(hWnd, profile);
}

bool MagService::_CheckSrcWnd(HWND hWnd) noexcept {
	if (!hWnd || !IsWindow(hWnd)) {
		return false;
	}

	UINT showCmd = Win32Utils::GetWindowShowCmd(hWnd);
	if (showCmd == SW_NORMAL) {
		return true;
	}

	return showCmd == SW_MAXIMIZE && AppSettings::Get().IsAllowScalingMaximized();
}

}
