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

using namespace winrt::Magpie;
using namespace winrt;
using namespace Windows::System::Threading;

namespace Magpie {

ScalingService& ScalingService::Get() noexcept {
	static ScalingService instance;
	return instance;
}

ScalingService::~ScalingService() {}

void ScalingService::Initialize() {
	_dispatcher = CoreWindow::GetForCurrentThread().Dispatcher();
	
	_countDownTimer.Interval(25ms);
	_countDownTimer.Tick({ this, &ScalingService::_CountDownTimer_Tick });

	_checkForegroundTimer = ThreadPoolTimer::CreatePeriodicTimer(
		{ this, &ScalingService::_CheckForegroundTimer_Tick },
		50ms
	);
	
	_isAutoRestoreChangedRevoker = AppSettings::Get().IsAutoRestoreChanged(
		auto_revoke, std::bind_front(&ScalingService::_Settings_IsAutoRestoreChanged, this));
	_scalingRuntime = std::make_unique<ScalingRuntime>();
	_scalingRuntime->IsRunningChanged(
		std::bind_front(&ScalingService::_ScalingRuntime_IsRunningChanged, this));

	_shortcutActivatedRevoker = ShortcutService::Get().ShortcutActivated(
		auto_revoke, std::bind_front(&ScalingService::_ShortcutService_ShortcutPressed, this));

	// 立即检查前台窗口
	_CheckForegroundTimer_Tick(nullptr);
}

void ScalingService::Uninitialize() {
	_checkForegroundTimer.Cancel();
	_countDownTimer.Stop();
	_scalingRuntime.reset();

	_isAutoRestoreChangedRevoker.Revoke();
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

static void ShowError(HWND hWnd, ScalingError error) noexcept {
	const wchar_t* key = nullptr;

	switch (error) {
	case ScalingError::InvalidScalingMode:
		key = L"Message_InvalidScalingMode"; break;
	case ScalingError::TouchSupport:
		key = L"Message_TouchSupport"; break;
	case ScalingError::InvalidSourceWindow:
		key = L"Message_InvalidSourceWindow"; break;
	// ScalingError::SystemWindow 错误无需显示消息
	case ScalingError::Maximized:
		key = L"Message_Maximized"; break;
	case ScalingError::LowIntegrityLevel:
		key = L"Message_LowIntegrityLevel"; break;
	case ScalingError::ScalingFailedGeneral:
		key = L"Message_ScalingFailedGeneral"; break;
	case ScalingError::CaptureFailed:
		key = L"Message_CaptureFailed"; break;
	case ScalingError::CreateFenceFailed:
		key = L"Message_CreateFenceFailed"; break;
	default:
		return;
	}

	ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	hstring title = error == ScalingError::Maximized ? hstring{} : resourceLoader.GetString(L"Message_ScalingFailed");
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
		co_await _dispatcher;
	}

	if (_hwndToRestore == hwndFore) {
		// 检查自动恢复
		if (ScalingError error = _CheckSrcWnd(hwndFore, false); error == ScalingError::NoError) {
			const Profile* profile = ProfileService::Get().GetProfileForWindow(hwndFore, false);
			_StartScale(hwndFore, *profile);
			co_return;
		} else {
			ShowError(hwndFore, error);
		}

		// _hwndToRestore 无法缩放则清空
		_WndToRestore(NULL);
	} else {
		// 检查自动缩放
		if (const Profile* profile = ProfileService::Get().GetProfileForWindow(hwndFore, true)) {
			ScalingError error = _CheckSrcWnd(hwndFore, true);
			if (error == ScalingError::NoError) {
				_StartScale(hwndFore, *profile);
				co_return;
			} else {
				ShowError(hwndFore, error);
			}
		}
		
		if (_hwndToRestore && _CheckSrcWnd(_hwndToRestore, false) != ScalingError::NoError) {
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

void ScalingService::_ScalingRuntime_IsRunningChanged(bool isRunning, ScalingError error) {
	_dispatcher.RunAsync(CoreDispatcherPriority::Normal, [this, isRunning, error]() {
		if (isRunning) {
			StopTimer();

			if (AppSettings::Get().IsAutoRestore()) {
				_WndToRestore(NULL);
			}
		} else {
			if (error != ScalingError::NoError && IsWindowVisible(_hwndCurSrc)) {
				// 缩放初始化时或缩放中途出错
				ShowError(_hwndCurSrc, error);
			}

			if (GetForegroundWindow() == _hwndCurSrc) {
				// 退出全屏后如果前台窗口不变视为通过热键退出
				_hwndChecked = _hwndCurSrc;
			} else if (!_isAutoScaling && AppSettings::Get().IsAutoRestore()) {
				// 无需再次检查完整性级别
				if (_CheckSrcWnd(_hwndCurSrc, false) == ScalingError::NoError) {
					_WndToRestore(_hwndCurSrc);
				}
			}

			_hwndCurSrc = NULL;

			// 立即检查前台窗口
			_CheckForegroundTimer_Tick(nullptr);
		}

		IsRunningChanged.Invoke(isRunning);
	});
}

void ScalingService::_StartScale(HWND hWnd, const Profile& profile) {
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
	
	options.graphicsCard = profile.graphicsCard;
	options.captureMethod = profile.captureMethod;
	if (profile.isFrameRateLimiterEnabled) {
		options.maxFrameRate = profile.maxFrameRate;
	}
	options.multiMonitorUsage = profile.multiMonitorUsage;
	options.cursorInterpolationMode = profile.cursorInterpolationMode;
	options.flags = profile.scalingFlags;

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
}

void ScalingService::_ScaleForegroundWindow() {
	HWND hWnd = GetForegroundWindow();
	if (ScalingError error = _CheckSrcWnd(hWnd, true); error != ScalingError::NoError) {
		ShowError(hWnd, error);
		return;
	}

	const Profile& profile = *ProfileService::Get().GetProfileForWindow(hWnd, false);
	_StartScale(hWnd, profile);
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

ScalingError ScalingService::_CheckSrcWnd(HWND hWnd, bool checkIL) noexcept {
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

	if (checkIL) {
		// 禁止缩放完整性级别 (integrity level) 更高的窗口
		static DWORD thisIL = []() -> DWORD {
			DWORD il;
			return Win32Helper::GetProcessIntegrityLevel(NULL, il) ? il : 0;
		}();

		DWORD windowIL;
		if (!GetWindowIntegrityLevel(hWnd, windowIL) || windowIL > thisIL) {
			return ScalingError::LowIntegrityLevel;
		}
	}
	
	return ScalingError::NoError;
}

}
