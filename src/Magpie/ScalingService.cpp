#include "pch.h"
#include "ScalingService.h"
#include "ShortcutService.h"
#include "Win32Helper.h"
#include "AppSettings.h"
#include "ProfileService.h"
#include "ScalingModesService.h"
#include "ScalingMode.h"
#include "Logger.h"
#include "EffectsService.h"
#include "TouchHelper.h"
#include "ToastService.h"
#include "CommonSharedConstants.h"
#include "ScalingRuntime.h"
#include "WindowHelper.h"
#include "App.h"

using namespace winrt::Magpie::implementation;
using namespace winrt;
using namespace Windows::System::Threading;

using winrt::Magpie::ShortcutAction;

namespace Magpie {

ScalingService& ScalingService::Get() noexcept {
	static ScalingService instance;
	return instance;
}

ScalingService::~ScalingService() {}

void ScalingService::Initialize() {
	_countDownTimer.Interval(25ms);
	_countDownTimer.Tick({ this, &ScalingService::_CountDownTimer_Tick });

	_checkForegroundTimer = ThreadPoolTimer::CreatePeriodicTimer(
		{ this, &ScalingService::_CheckForegroundTimer_Tick },
		50ms
	);
	
	_scalingRuntime = std::make_unique<ScalingRuntime>();
	_scalingRuntime->IsRunningChanged(
		std::bind_front(&ScalingService::_ScalingRuntime_IsRunningChanged, this));

	_shortcutActivatedRevoker = ShortcutService::Get().ShortcutActivated(
		auto_revoke, std::bind_front(&ScalingService::_ShortcutService_ShortcutPressed, this));

	// 立即检查前台窗口
	_CheckForegroundTimer_Tick(nullptr);
}

void ScalingService::Uninitialize() {
	if (!_checkForegroundTimer) {
		return;
	}

	_checkForegroundTimer.Cancel();
	_countDownTimer.Stop();
	_scalingRuntime.reset();

	_shortcutActivatedRevoker.Revoke();
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

bool ScalingService::IsRunning() const noexcept {
	return _scalingRuntime && _scalingRuntime->IsRunning();
}

void ScalingService::CheckForeground() {
	_hwndChecked = NULL;
	_CheckForegroundTimer_Tick(nullptr);
}

void ScalingService::_ShortcutService_ShortcutPressed(ShortcutAction action) {
	if (!_scalingRuntime) {
		return;
	}

	switch (action) {
	case ShortcutAction::Scale:
	case ShortcutAction::WindowedModeScale:
	{
		if (_scalingRuntime->IsRunning()) {
			_scalingRuntime->Stop();
			return;
		}

		_ScaleForegroundWindow(action == ShortcutAction::WindowedModeScale);
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

void ScalingService::_CountDownTimer_Tick(winrt::IInspectable const&, winrt::IInspectable const&) {
	double timeLeft = SecondsLeft();

	// 剩余时间在 10 ms 以内计时结束
	if (timeLeft < 0.01) {
		StopTimer();
		_ScaleForegroundWindow(false);
		return;
	}

	TimerTick.Invoke(timeLeft);
}

static void ShowError(HWND hWnd, ScalingError error) noexcept {
	const wchar_t* key = nullptr;

	bool isFail = true;
	switch (error) {
	case ScalingError::InvalidScalingMode:
		key = L"Message_InvalidScalingMode";
		isFail = false;
		break;
	case ScalingError::TouchSupport:
		key = L"Message_TouchSupport";
		break;
	case ScalingError::InvalidSourceWindow:
		key = L"Message_InvalidSourceWindow";
		break;
	// ScalingError::SystemWindow 错误无需显示消息
	case ScalingError::Maximized:
		key = L"Message_Maximized";
		isFail = false;
		break;
	case ScalingError::LowIntegrityLevel:
		key = L"Message_LowIntegrityLevel";
		isFail = false;
		break;
	case ScalingError::ScalingFailedGeneral:
		key = L"Message_ScalingFailedGeneral";
		break;
	case ScalingError::CaptureFailed:
		key = L"Message_CaptureFailed";
		break;
	case ScalingError::CreateFenceFailed:
		key = L"Message_CreateFenceFailed";
		break;
	default:
		return;
	}

	ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	hstring title = isFail ? resourceLoader.GetString(L"Message_ScalingFailed") : hstring{};
	ToastService::Get().ShowMessageOnWindow(title, resourceLoader.GetString(key), hWnd);
	Logger::Get().Error(fmt::format("缩放失败\n\t错误码: {}", (int)error));
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
		co_await App::Get().Dispatcher();
	}

	// 检查自动缩放
	if (const Profile* profile = ProfileService::Get().GetProfileForWindow(hwndFore, true)) {
		ScalingError error = _CheckSrcWnd(hwndFore);
		if (error == ScalingError::NoError) {
			_StartScale(hwndFore, *profile, false);
			co_return;
		} else {
			ShowError(hwndFore, error);
		}
	}

	// 避免重复检查
	_hwndChecked = hwndFore;
}

void ScalingService::_ScalingRuntime_IsRunningChanged(bool isRunning, ScalingError error) {
	App::Get().Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [this, isRunning, error]() {
		if (isRunning) {
			StopTimer();
		} else {
			if (error != ScalingError::NoError && IsWindowVisible(_hwndCurSrc)) {
				// 缩放初始化时或缩放中途出错
				ShowError(_hwndCurSrc, error);
			}

			if (GetForegroundWindow() == _hwndCurSrc) {
				// 退出全屏后如果前台窗口不变视为通过热键退出
				_hwndChecked = _hwndCurSrc;
			}

			_hwndCurSrc = NULL;

			// 立即检查前台窗口
			_CheckForegroundTimer_Tick(nullptr);
		}

		IsRunningChanged.Invoke(isRunning);
	});
}

void ScalingService::_StartScale(HWND hWnd, const Profile& profile, bool windowedMode) {
	if (profile.scalingMode < 0) {
		ShowError(hWnd, ScalingError::InvalidScalingMode);
		return;
	}

	ScalingOptions options;
	options.effects = ScalingModesService::Get().GetScalingMode(profile.scalingMode).effects;
	if (options.effects.empty()) {
		ShowError(hWnd, ScalingError::InvalidScalingMode);
		return;
	} else {
		for (EffectOption& effect : options.effects) {
			if (!EffectsService::Get().GetEffect(effect.name)) {
				// 存在无法解析的效果
				ShowError(hWnd, ScalingError::InvalidScalingMode);
				return;
			}
		}
	}

	// 尝试启用触控支持
	bool isTouchSupportEnabled;
	if (!TouchHelper::TryLaunchTouchHelper(isTouchSupportEnabled)) {
		Logger::Get().Error("TryLaunchTouchHelper 失败");
		ShowError(hWnd, ScalingError::TouchSupport);
		return;
	}
	
	options.graphicsCardId = profile.graphicsCardId;
	options.captureMethod = profile.captureMethod;
	if (profile.isFrameRateLimiterEnabled) {
		options.maxFrameRate = profile.maxFrameRate;
	}
	options.multiMonitorUsage = profile.multiMonitorUsage;
	options.cursorInterpolationMode = profile.cursorInterpolationMode;
	options.flags = profile.scalingFlags;

	options.IsWindowedMode(windowedMode);
	options.IsTouchSupportEnabled(isTouchSupportEnabled);

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
	options.IsDeveloperMode(settings.IsDeveloperMode());
	options.IsDebugMode(settings.IsDebugMode());
	options.IsBenchmarkMode(settings.IsBenchmarkMode());
	options.IsEffectCacheDisabled(settings.IsEffectCacheDisabled());
	options.IsFontCacheDisabled(settings.IsFontCacheDisabled());
	options.IsSaveEffectSources(settings.IsSaveEffectSources());
	options.IsWarningsAreErrors(settings.IsWarningsAreErrors());
	options.IsAllowScalingMaximized(settings.IsAllowScalingMaximized());
	options.IsSimulateExclusiveFullscreen(settings.IsSimulateExclusiveFullscreen());
	options.duplicateFrameDetectionMode = settings.DuplicateFrameDetectionMode();
	options.IsStatisticsForDynamicDetectionEnabled(settings.IsStatisticsForDynamicDetectionEnabled());
	options.IsInlineParams(settings.IsInlineParams());
	options.IsFP16Disabled(settings.IsFP16Disabled());
	
	if (options.maxFrameRate) {
		// 最小帧数不能大于最大帧数
		options.minFrameRate = std::min(settings.MinFrameRate(), *options.maxFrameRate);
	} else {
		options.minFrameRate = settings.MinFrameRate();
	}

	_isAutoScaling = profile.isAutoScale;
	_scalingRuntime->Start(hWnd, std::move(options));
	_hwndCurSrc = hWnd;
}

void ScalingService::_ScaleForegroundWindow(bool windowedMode) {
	HWND hWnd = GetForegroundWindow();
	if (ScalingError error = _CheckSrcWnd(hWnd); error != ScalingError::NoError) {
		ShowError(hWnd, error);
		return;
	}

	const Profile& profile = *ProfileService::Get().GetProfileForWindow(hWnd, false);
	_StartScale(hWnd, profile, windowedMode);
}

static bool GetWindowIntegrityLevel(HWND hWnd, DWORD& integrityLevel) noexcept {
	wil::unique_process_handle hProc = Win32Helper::GetWndProcessHandle(hWnd);
	if (!hProc) {
		Logger::Get().Error("GetWndProcessHandle 失败");
		return false;
	}

	wil::unique_handle hQueryToken;
	if (!OpenProcessToken(hProc.get(), TOKEN_QUERY, hQueryToken.put())) {
		Logger::Get().Win32Error("OpenProcessToken 失败");
		return false;
	}

	return Win32Helper::GetProcessIntegrityLevel(hQueryToken.get(), integrityLevel);
}

ScalingError ScalingService::_CheckSrcWnd(HWND hWnd) noexcept {
	if (!hWnd || !IsWindowVisible(hWnd)) {
		return ScalingError::InvalidSourceWindow;
	}

	// 不缩放不接受点击的窗口
	if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT) {
		return ScalingError::InvalidSourceWindow;
	}

	if (WindowHelper::IsForbiddenSystemWindow(hWnd)) {
		return ScalingError::SystemWindow;
	}

	// 不缩放最小化的窗口，是否缩放最大化的窗口由设置决定
	if (UINT showCmd = Win32Helper::GetWindowShowCmd(hWnd); showCmd != SW_NORMAL) {
		if (showCmd != SW_MAXIMIZE) {
			return ScalingError::InvalidSourceWindow;
		}

		if (!AppSettings::Get().IsAllowScalingMaximized()) {
			return ScalingError::Maximized;
		}
	}

	// 不缩放过小的窗口
	{
		RECT clientRect;
		if (!GetClientRect(hWnd, &clientRect)) {
			Logger::Get().Win32Error("GetClientRect 失败");
			return ScalingError::InvalidSourceWindow;
		}

		const SIZE clientSize = Win32Helper::GetSizeOfRect(clientRect);
		if (clientSize.cx < 64 || clientSize.cy < 64) {
			return ScalingError::InvalidSourceWindow;
		}
	}

	// 禁止缩放完整性级别 (integrity level) 更高的窗口
	static DWORD thisIL = []() -> DWORD {
		DWORD il;
		return Win32Helper::GetProcessIntegrityLevel(NULL, il) ? il : 0;
	}();

	DWORD windowIL;
	if (!GetWindowIntegrityLevel(hWnd, windowIL) || windowIL > thisIL) {
		return ScalingError::LowIntegrityLevel;
	}
	
	return ScalingError::NoError;
}

}
