#include "pch.h"
#include "App.h"
#include "AppSettings.h"
#include "CommonSharedConstants.h"
#include "EffectsService.h"
#include "Logger.h"
#include "ProfileService.h"
#include "ScalingMode.h"
#include "ScalingModesService.h"
#include "ScalingRuntime.h"
#include "ScalingService.h"
#include "ShortcutService.h"
#include "ToastService.h"
#include "TouchHelper.h"
#include "Win32Helper.h"
#include "WindowHelper.h"

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
	_scalingRuntime = std::make_unique<ScalingRuntime>();
	_scalingRuntime->IsScalingChanged(
		std::bind_front(&ScalingService::_ScalingRuntime_IsScalingChanged, this));

	_countDownTimer.Interval(25ms);
	_countDownTimer.Tick({ this, &ScalingService::_CountDownTimer_Tick });

	_checkForegroundTimer = ThreadPoolTimer::CreatePeriodicTimer(
		{ this, &ScalingService::_CheckForegroundTimer_Tick },
		50ms
	);
	
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

bool ScalingService::IsScaling() const noexcept {
	return _scalingRuntime->IsScaling();
}

void ScalingService::CheckForeground() {
	_hwndChecked = NULL;
	_CheckForegroundTimer_Tick(nullptr);
}

void ScalingService::_ShortcutService_ShortcutPressed(ShortcutAction action) {
	switch (action) {
	case ShortcutAction::Scale:
	case ShortcutAction::WindowedModeScale:
	{
		const bool isWindowdMode = action == ShortcutAction::WindowedModeScale;

		if (_scalingRuntime->IsScaling()) {
			_scalingRuntime->SwitchScalingState(isWindowdMode);
		} else {
			_ScaleForegroundWindow(isWindowdMode);
		}

		break;
	}
	case ShortcutAction::Toolbar:
	{
		if (_scalingRuntime->IsScaling()) {
			_scalingRuntime->SwitchToolbarState();
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
	case ScalingError::Windowed3DGameMode:
		key = L"Message_Windowed3DGameMode";
		isFail = false;
		break;
	case ScalingError::InvalidSourceWindow:
		key = L"Message_InvalidSourceWindow";
		break;
	case ScalingError::Maximized:
		key = L"Message_Maximized";
		isFail = false;
		break;
	case ScalingError::LowIntegrityLevel:
		key = L"Message_LowIntegrityLevel";
		isFail = false;
		break;
	case ScalingError::InvalidCropping:
		key = L"Message_InvalidCropping";
		break;
	case ScalingError::BannedInWindowedMode:
		key = L"Message_BannedInWindowedMode";
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
		assert(false);
		return;
	}

	ResourceLoader resourceLoader =
		ResourceLoader::GetForViewIndependentUse(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	hstring title = isFail ? resourceLoader.GetString(L"Message_ScalingFailed") : hstring{};
	ToastService::Get().ShowMessageOnWindow(title, resourceLoader.GetString(key), hWnd);
	Logger::Get().Error(fmt::format("缩放失败\n\t错误码: {}", (int)error));
}

static bool IsReadyForScaling(HWND hwndFore) noexcept {
	// GH#538
	// 窗口还原过程中存在中间状态：虽然已经成为前台窗口，但仍是最小化的
	if (Win32Helper::GetWindowShowCmd(hwndFore) == SW_SHOWMINIMIZED) {
		return false;
	}

	// GH#1148
	// 有些游戏刚启动时将窗口创建在屏幕外，初始化完成后再移到屏幕内
	if (!MonitorFromWindow(hwndFore, MONITOR_DEFAULTTONULL)) {
		return false;
	}

	// GH#1200
	// 有些游戏加载时不响应消息，应等待加载完成
	return !Win32Helper::IsWindowHung(hwndFore);
}

fire_and_forget ScalingService::_CheckForegroundTimer_Tick(ThreadPoolTimer const& timer) {
	if (_scalingRuntime->IsScaling()) {
		co_return;
	}

	if (timer) {
		// ThreadPoolTimer 在后台线程触发
		co_await App::Get().Dispatcher();
	}

	const HWND hwndFore = GetForegroundWindow();
	if (!hwndFore || hwndFore == _hwndChecked) {
		co_return;
	}

	// 检查自动缩放
	if (const Profile* profile = ProfileService::Get().GetProfileForWindow(hwndFore, true)) {
		// 如果窗口处于某种中间状态则跳过此次检查
		if (!IsReadyForScaling(hwndFore)) {
			co_return;
		}

		_StartScale(hwndFore, *profile, profile->autoScale == AutoScale::Windowed);
	}

	// 避免重复检查
	_hwndChecked = hwndFore;
}

void ScalingService::_ScalingRuntime_IsScalingChanged(bool value) {
	App::Get().Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [this, value]() {
		if (value) {
			StopTimer();
		} else {
			if (GetForegroundWindow() == _hwndCurSrc) {
				// 退出全屏后如果前台窗口不变视为通过热键退出
				_hwndChecked = _hwndCurSrc;
			}

			_hwndCurSrc = NULL;

			// 立即检查前台窗口
			_CheckForegroundTimer_Tick(nullptr);
		}

		IsScalingChanged.Invoke(value);
	});
}

void ScalingService::_ScaleForegroundWindow(bool windowedMode) {
	const HWND hWnd = GetForegroundWindow();
	if (!hWnd) {
		return;
	}

	const Profile& profile = *ProfileService::Get().GetProfileForWindow(hWnd, false);
	_StartScale(hWnd, profile, windowedMode);
}

void ScalingService::_StartScale(HWND hWnd, const Profile& profile, bool windowedMode) {
	assert(hWnd);

	if (_scalingRuntime->IsScaling()) {
		return;
	}

	const ScalingError error = _StartScaleImpl(hWnd, profile, windowedMode);
	if (error != ScalingError::NoError) {
		ShowError(hWnd, error);
	}
}

ScalingError ScalingService::_StartScaleImpl(HWND hWnd, const Profile& profile, bool windowedMode) {
	if (WindowHelper::IsForbiddenSystemWindow(hWnd)) {
		// 不显示错误
		return ScalingError::NoError;
	}

	if (profile.scalingMode < 0) {
		return ScalingError::InvalidScalingMode;
	}

	const std::vector<EffectItem>& effects =
		ScalingModesService::Get().GetScalingMode(profile.scalingMode).effects;
	if (effects.empty()) {
		return ScalingError::InvalidScalingMode;
	} else {
		for (const EffectItem& effect : effects) {
			if (!EffectsService::Get().GetEffect(effect.name)) {
				// 存在无法解析的效果
				return ScalingError::InvalidScalingMode;
			}
		}
	}

	if (profile.Is3DGameMode() && windowedMode) {
		return ScalingError::Windowed3DGameMode;
	}

	ScalingOptions options;

	options.effects.reserve(effects.size());
	for (const EffectItem& effectItem : effects) {
		options.effects.push_back((EffectOption)effectItem);
	}

	// 尝试启用触控支持
	bool isTouchSupportEnabled;
	if (!TouchHelper::TryLaunchTouchHelper(isTouchSupportEnabled)) {
		Logger::Get().Error("TryLaunchTouchHelper 失败");
		return ScalingError::TouchSupport;
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

	if (windowedMode) {
		switch (profile.initialWindowedScaleFactor) {
		case InitialWindowedScaleFactor::Auto:
			options.initialWindowedScaleFactor = 0.0f;
			break;
		case InitialWindowedScaleFactor::x1_25:
			options.initialWindowedScaleFactor = 1.25f;
			break;
		case InitialWindowedScaleFactor::x1_5:
			options.initialWindowedScaleFactor = 1.5f;
			break;
		case InitialWindowedScaleFactor::x1_75:
			options.initialWindowedScaleFactor = 1.5f;
			break;
		case InitialWindowedScaleFactor::x2:
			options.initialWindowedScaleFactor = 2.0f;
			break;
		case InitialWindowedScaleFactor::x3:
			options.initialWindowedScaleFactor = 3.0f;
			break;
		case InitialWindowedScaleFactor::Custom:
			options.initialWindowedScaleFactor = profile.customInitialWindowedScaleFactor;
			break;
		default:
			options.initialWindowedScaleFactor = 0.0f;
			break;
		}
	}

	if (profile.isCroppingEnabled) {
		options.cropping = profile.cropping;
	}

	switch (profile.cursorScaling) {
	case CursorScaling::x0_5:
		options.cursorScaling = 0.5f;
		break;
	case CursorScaling::x0_75:
		options.cursorScaling = 0.75f;
		break;
	case CursorScaling::NoScaling:
		options.cursorScaling = 1.0f;
		break;
	case CursorScaling::x1_25:
		options.cursorScaling = 1.25f;
		break;
	case CursorScaling::x1_5:
		options.cursorScaling = 1.5f;
		break;
	case CursorScaling::x2:
		options.cursorScaling = 2.0f;
		break;
	case CursorScaling::Source:
		// 0 或负值表示和源窗口缩放比例相同
		options.cursorScaling = 0.0f;
		break;
	case CursorScaling::Custom:
		options.cursorScaling = profile.customCursorScaling;
		break;
	default:
		options.cursorScaling = 1.0f;
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

	options.fullscreenInitialToolbarState = settings.FullscreenInitialToolbarState();
	options.windowedInitialToolbarState = settings.WindowedInitialToolbarState();
	options.screenshotsDir = settings.ScreenshotsDir();
	if (options.screenshotsDir.empty()) {
		// 回落到使用当前目录
		options.screenshotsDir = L".";
	}

	options.overlayOptions = settings.OverlayOptions();

	options.showToast = [](HWND hwndTarget, std::wstring_view msg) noexcept {
		ToastService::Get().ShowMessageOnWindow({}, msg, hwndTarget);
	};

	options.showError = &ShowError;

	options.save = [](const ScalingOptions& options, HWND /*hwndScaling*/) noexcept {
		App::Get().Dispatcher().RunAsync(
			CoreDispatcherPriority::Normal,
			[overlayOptions(options.overlayOptions)]() {
				AppSettings::Get().OverlayOptions() = std::move(overlayOptions);
				AppSettings::Get().SaveAsync();
			}
		);
	};

	if (!_scalingRuntime->Start(hWnd, std::move(options))) {
		return ScalingError::ScalingFailedGeneral;
	}

	_hwndCurSrc = hWnd;
	return ScalingError::NoError;
}

}
