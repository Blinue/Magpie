#include "pch.h"
#include "ScalingService.h"
#include "ShortcutService.h"
#include "Win32Utils.h"
#include "AppSettings.h"
#include "ProfileService.h"
#include "ScalingModesService.h"
#include "ScalingMode.h"
#include "Logger.h"
#include "EffectsService.h"
#include <Magpie.Core.h>

using namespace ::Magpie::Core;
using namespace winrt;
using namespace Windows::System::Threading;

namespace winrt::Magpie::App {

void ScalingService::Initialize() {
	_dispatcher = CoreWindow::GetForCurrentThread().Dispatcher();
	
	_countDownTimer.Interval(25ms);
	_countDownTimer.Tick({ this, &ScalingService::_CountDownTimer_Tick });

	_checkForegroundTimer = ThreadPoolTimer::CreatePeriodicTimer(
		{ this, &ScalingService::_CheckForegroundTimer_Tick },
		50ms
	);
	
	AppSettings::Get().IsAutoRestoreChanged({ this, &ScalingService::_Settings_IsAutoRestoreChanged });
	_scalingRuntime = std::make_unique<ScalingRuntime>();
	_scalingRuntime->IsRunningChanged({ this, &ScalingService::_ScalingRuntime_IsRunningChanged });

	ShortcutService::Get().ShortcutActivated(
		{ this, &ScalingService::_ShortcutService_ShortcutPressed }
	);

	// 立即检查前台窗口
	_CheckForegroundTimer_Tick(nullptr);
}

void ScalingService::Uninitialize() {
	_checkForegroundTimer.Cancel();
	_countDownTimer.Stop();
	_scalingRuntime.reset();
}

void ScalingService::StartTimer() {
	if (_curCountdownSeconds != 0) {
		return;
	}

	_curCountdownSeconds = AppSettings::Get().CountdownSeconds();
	_timerStartTimePoint = std::chrono::steady_clock::now();
	_countDownTimer.Start();
	IsTimerOnChanged.Invoke(true);
}

void ScalingService::StopTimer() {
	if (_curCountdownSeconds == 0) {
		return;
	}

	_curCountdownSeconds = 0;
	_countDownTimer.Stop();
	IsTimerOnChanged.Invoke(false);
}

double ScalingService::SecondsLeft() const noexcept {
	using namespace std::chrono;

	if (!IsTimerOn()) {
		return 0.0;
	}

	// DispatcherTimer 误差很大，因此我们自己计算剩余时间
	auto now = steady_clock::now();
	int msLeft = (int)duration_cast<milliseconds>(_timerStartTimePoint + seconds(_curCountdownSeconds) - now).count();
	return msLeft / 1000.0;
}

void ScalingService::ClearWndToRestore() {
	_WndToRestore(NULL);
}

bool ScalingService::IsRunning() const noexcept {
	return _scalingRuntime && _scalingRuntime->IsRunning();
}

void ScalingService::CheckForeground() {
	_hwndChecked = NULL;
	_CheckForegroundTimer_Tick(nullptr);
}

void ScalingService::_WndToRestore(HWND value) {
	if (_hwndToRestore == value) {
		return;
	}

	_hwndToRestore = value;
	WndToRestoreChanged.Invoke(_hwndToRestore);
}

void ScalingService::_ShortcutService_ShortcutPressed(ShortcutAction action) {
	if (!_scalingRuntime) {
		return;
	}

	switch (action) {
	case ShortcutAction::Scale:
	{
		if (_scalingRuntime->IsRunning()) {
			_scalingRuntime->Stop();
			return;
		}

		_ScaleForegroundWindow();
		break;
	}
	case ShortcutAction::Overlay:
	{
		if (_scalingRuntime->IsRunning()) {
			_scalingRuntime->ToggleOverlay();
			return;
		}
		break;
	}
	default:
		break;
	}
}

void ScalingService::_CountDownTimer_Tick(IInspectable const&, IInspectable const&) {
	double timeLeft = SecondsLeft();

	// 剩余时间在 10 ms 以内计时结束
	if (timeLeft < 0.01) {
		StopTimer();
		_ScaleForegroundWindow();
		return;
	}

	TimerTick.Invoke(timeLeft);
}

fire_and_forget ScalingService::_CheckForegroundTimer_Tick(ThreadPoolTimer const& timer) {
	if (!_scalingRuntime || _scalingRuntime->IsRunning()) {
		co_return;
	}

	HWND hwndFore = GetForegroundWindow();
	if (!hwndFore || hwndFore == _hwndChecked) {
		co_return;
	}
	_hwndChecked = NULL;

	if (timer) {
		// ThreadPoolTimer 在后台线程触发
		co_await _dispatcher;
	}

	if (_hwndToRestore == hwndFore) {
		// 检查自动恢复
		if (_CheckSrcWnd(hwndFore, false)) {
			const Profile* profile = ProfileService::Get().GetProfileForWindow(hwndFore, false);
			_StartScale(hwndFore, *profile);
			co_return;
		}

		// _hwndToRestore 无法缩放则清空
		_WndToRestore(NULL);
	} else {
		// 检查自动缩放
		const Profile* profile = ProfileService::Get().GetProfileForWindow(hwndFore, true);
		if (profile && _CheckSrcWnd(hwndFore, true)) {
			_StartScale(hwndFore, *profile);
			co_return;
		}
		
		if (_hwndToRestore && !_CheckSrcWnd(_hwndToRestore, false)) {
			// _hwndToRestore 无法缩放则清空
			_WndToRestore(NULL);
		}
	}

	// 避免重复检查
	_hwndChecked = hwndFore;
}

void ScalingService::_Settings_IsAutoRestoreChanged(bool value) {
	if (!value) {
		_WndToRestore(NULL);
	}
}

fire_and_forget ScalingService::_ScalingRuntime_IsRunningChanged(bool isRunning) {
	co_await _dispatcher;

	if (isRunning) {
		StopTimer();

		if (AppSettings::Get().IsAutoRestore()) {
			_WndToRestore(NULL);
		}
	} else {
		if (GetForegroundWindow() == _hwndCurSrc) {
			// 退出全屏后如果前台窗口不变视为通过热键退出
			_hwndChecked = _hwndCurSrc;
		} else if (!_isAutoScaling && AppSettings::Get().IsAutoRestore()) {
			// 无需再次检查完整性级别
			if (_CheckSrcWnd(_hwndCurSrc, false)) {
				_WndToRestore(_hwndCurSrc);
			}
		}

		_hwndCurSrc = NULL;

		// 立即检查前台窗口
		_CheckForegroundTimer_Tick(nullptr);
	}

	IsRunningChanged.Invoke(isRunning);
}

bool ScalingService::_StartScale(HWND hWnd, const Profile& profile) {
	if (profile.scalingMode < 0) {
		return false;
	}

	ScalingOptions options;
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
	if (profile.isFrameRateLimiterEnabled) {
		options.maxFrameRate = profile.maxFrameRate;
	}
	options.multiMonitorUsage = profile.multiMonitorUsage;
	options.cursorInterpolationMode = profile.cursorInterpolationMode;
	options.flags = profile.scalingFlags;

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

	options.IsDebugMode(settings.IsDebugMode());
	options.IsEffectCacheDisabled(settings.IsEffectCacheDisabled());
	options.IsFontCacheDisabled(settings.IsFontCacheDisabled());
	options.IsSaveEffectSources(settings.IsSaveEffectSources());
	options.IsWarningsAreErrors(settings.IsWarningsAreErrors());
	options.IsAllowScalingMaximized(settings.IsAllowScalingMaximized());
	options.IsSimulateExclusiveFullscreen(settings.IsSimulateExclusiveFullscreen());
	options.duplicateFrameDetectionMode = settings.DuplicateFrameDetectionMode();
	options.IsStatisticsForDynamicDetectionEnabled(settings.IsStatisticsForDynamicDetectionEnabled());

	_isAutoScaling = profile.isAutoScale;
	_scalingRuntime->Start(hWnd, std::move(options));
	_hwndCurSrc = hWnd;
	return true;
}

void ScalingService::_ScaleForegroundWindow() {
	HWND hWnd = GetForegroundWindow();
	if (!_CheckSrcWnd(hWnd, true)) {
		return;
	}

	const Profile& profile = *ProfileService::Get().GetProfileForWindow(hWnd, false);
	_StartScale(hWnd, profile);
}

static bool GetWindowIntegrityLevel(HWND hWnd, DWORD& integrityLevel) noexcept {
	DWORD processId;
	if (!GetWindowThreadProcessId(hWnd, &processId)) {
		Logger::Get().Win32Error("GetWindowThreadProcessId 失败");
		return false;
	}

	wil::unique_process_handle hProc(
		OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId));
	if (!hProc) {
		Logger::Get().Win32Error("OpenProcess 失败");
		return false;
	}

	wil::unique_handle hQueryToken;
	if (!OpenProcessToken(hProc.get(), TOKEN_QUERY, hQueryToken.put())) {
		Logger::Get().Win32Error("OpenProcessToken 失败");
		return false;
	}

	return Win32Utils::GetProcessIntegrityLevel(hQueryToken.get(), integrityLevel);
}

bool ScalingService::_CheckSrcWnd(HWND hWnd, bool checkIL) noexcept {
	if (!hWnd || !IsWindow(hWnd)) {
		return false;
	}

	if (!WindowHelper::IsValidSrcWindow(hWnd)) {
		return false;
	}

	// 不缩放最小化的窗口，是否缩放最大化的窗口由设置决定
	if (UINT showCmd = Win32Utils::GetWindowShowCmd(hWnd); showCmd != SW_NORMAL) {
		if (showCmd != SW_MAXIMIZE || !AppSettings::Get().IsAllowScalingMaximized()) {
			return false;
		}
	}

	// 不缩放过小的窗口
	{
		RECT clientRect;
		if (!GetClientRect(hWnd, &clientRect)) {
			return false;
		}

		const SIZE clientSize = Win32Utils::GetSizeOfRect(clientRect);
		if (clientSize.cx < 32 && clientSize.cy < 32) {
			return false;
		}
	}

	if (checkIL) {
		// 禁止缩放完整性级别 (integrity level) 更高的窗口
		static DWORD thisIL = []() -> DWORD {
			DWORD il;
			return Win32Utils::GetProcessIntegrityLevel(NULL, il) ? il : 0;
		}();

		DWORD windowIL;
		if (!GetWindowIntegrityLevel(hWnd, windowIL) || windowIL > thisIL) {
			return false;
		}
	}
	
	return true;
}

}
