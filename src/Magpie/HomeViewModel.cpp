#include "pch.h"
#include "HomeViewModel.h"
#if __has_include("HomeViewModel.g.cpp")
#include "HomeViewModel.g.cpp"
#endif
#include "AppSettings.h"
#include "ScalingService.h"
#include "Win32Helper.h"
#include "StrHelper.h"
#include "UpdateService.h"
#include "CommonSharedConstants.h"
#include "TouchHelper.h"
#include "LocalizationService.h"
#include "App.h"
#include "RootPage.h"

using namespace Magpie;

namespace winrt::Magpie::implementation {

HomeViewModel::HomeViewModel() {
	ScalingService& ScalingService = ScalingService::Get();

	_isRunningChangedRevoker = ScalingService.IsRunningChanged(
		auto_revoke, std::bind_front(&HomeViewModel::_ScalingService_IsRunningChanged, this));
	_isTimerOnRevoker = ScalingService.IsTimerOnChanged(
		auto_revoke, std::bind_front(&HomeViewModel::_ScalingService_IsTimerOnChanged, this));
	_timerTickRevoker = ScalingService.TimerTick(
		auto_revoke, std::bind_front(&HomeViewModel::_ScalingService_TimerTick, this));

	UpdateService& updateService = UpdateService::Get();
	_showUpdateCard = updateService.IsShowOnHomePage();
	_isShowOnHomePageChangedRevoker = updateService.IsShowOnHomePageChanged(
		auto_revoke,
		[this](bool value) {
			if (value) {
				ShowUpdateCard(true);
			}
		}
	);
}

bool HomeViewModel::IsTimerOn() const noexcept {
	return ScalingService::Get().IsTimerOn();
}

double HomeViewModel::TimerProgressRingValue() const noexcept {
	ScalingService& ScalingService = ScalingService::Get();
	return ScalingService.IsTimerOn() ? ScalingService.TimerProgress() : 1.0f;
}

hstring HomeViewModel::TimerLabelText() const noexcept {
	ScalingService& ScalingService = ScalingService::Get();
	return to_hstring((int)std::ceil(ScalingService.SecondsLeft()));
}

hstring HomeViewModel::TimerButtonText() const noexcept {
	ScalingService& ScalingService = ScalingService::Get();
	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	if (ScalingService.IsTimerOn()) {
		return resourceLoader.GetString(L"Home_Activation_Timer_Cancel");
	} else {
		hstring fmtStr = resourceLoader.GetString(L"Home_Activation_Timer_ButtonText");
		return hstring(fmt::format(
			fmt::runtime(std::wstring_view(fmtStr)),
			AppSettings::Get().CountdownSeconds()
		));
	}
}

bool HomeViewModel::IsNotRunning() const noexcept {
	return !ScalingService::Get().IsRunning();
}

void HomeViewModel::ToggleTimer() const noexcept {
	ScalingService& scalingService = ScalingService::Get();
	if (scalingService.IsTimerOn()) {
		scalingService.StopTimer();
	} else {
		scalingService.StartTimer();
	}
}

uint32_t HomeViewModel::Delay() const noexcept {
	return AppSettings::Get().CountdownSeconds();
}

void HomeViewModel::Delay(uint32_t value) {
	AppSettings::Get().CountdownSeconds(value);
	RaisePropertyChanged(L"Delay");
	RaisePropertyChanged(L"TimerButtonText");
}

inline void HomeViewModel::ShowUpdateCard(bool value) noexcept {
	_showUpdateCard = value;
	if (!value) {
		UpdateService::Get().IsShowOnHomePage(false);
	}
	
	RaisePropertyChanged(L"ShowUpdateCard");
	RaisePropertyChanged(L"UpdateCardTitle");
}

hstring HomeViewModel::UpdateCardTitle() const noexcept {
	UpdateService& updateService = UpdateService::Get();
	if (updateService.Status() < UpdateStatus::Available) {
		return {};
	}

	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	hstring titleFmt = resourceLoader.GetString(L"About_Version_UpdateCard_Title");
	return hstring(fmt::format(fmt::runtime(std::wstring_view(titleFmt)), updateService.Tag()));
}

bool HomeViewModel::IsAutoCheckForUpdates() const noexcept {
	return AppSettings::Get().IsAutoCheckForUpdates();
}

void HomeViewModel::IsAutoCheckForUpdates(bool value) noexcept {
	AppSettings::Get().IsAutoCheckForUpdates(value);
}

void HomeViewModel::DownloadAndInstall() {
	UpdateService::Get().DownloadAndInstall();
	App::Get().RootPage()->NavigateToAboutPage();
}

void HomeViewModel::ReleaseNotes() {
	std::wstring url = StrHelper::Concat(
		L"https://github.com/Blinue/Magpie/releases/tag/",
		UpdateService::Get().Tag()
	);
	Win32Helper::ShellOpen(url.c_str());
}

void HomeViewModel::RemindMeLater() {
	ShowUpdateCard(false);
}

bool HomeViewModel::IsTouchSupportEnabled() const noexcept {
	// 不检查版本号是否匹配
	return TouchHelper::IsTouchSupportEnabled();
}

fire_and_forget HomeViewModel::IsTouchSupportEnabled(bool value) {
	if (IsTouchSupportEnabled() == value) {
		co_return;
	}

	auto weakThis = get_weak();

	// UAC 可能导致 XAML Islands 崩溃，因此不能在主线程上执行 ShellExecute，
	// 见 https://github.com/microsoft/microsoft-ui-xaml/issues/4952
	co_await resume_background();

	TouchHelper::IsTouchSupportEnabled(value);

	co_await App::Get().Dispatcher();

	if (weakThis.get()) {
		RaisePropertyChanged(L"IsTouchSupportEnabled");
		RaisePropertyChanged(L"IsShowTouchSupportInfoBar");
	}
}

Uri HomeViewModel::TouchSupportLearnMoreUrl() const noexcept {
	if (LocalizationService::Get().Language() == L"zh-hans"sv) {
		return Uri(L"https://github.com/Blinue/Magpie/blob/dev/docs/%E5%85%B3%E4%BA%8E%E8%A7%A6%E6%8E%A7%E6%94%AF%E6%8C%81.md");
	} else {
		return Uri(L"https://github.com/Blinue/Magpie/blob/dev/docs/About%20touch%20support.md");
	}
}

bool HomeViewModel::IsShowTouchSupportInfoBar() const noexcept {
	return !Win32Helper::IsProcessElevated() && IsTouchSupportEnabled();
}

bool HomeViewModel::IsAllowScalingMaximized() const noexcept {
	return AppSettings::Get().IsAllowScalingMaximized();
}

void HomeViewModel::IsAllowScalingMaximized(bool value) {
	AppSettings::Get().IsAllowScalingMaximized(value);

	if (value) {
		ScalingService::Get().CheckForeground();
	}
}

bool HomeViewModel::IsInlineParams() const noexcept {
	return AppSettings::Get().IsInlineParams();
}

void HomeViewModel::IsInlineParams(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsInlineParams() == value) {
		return;
	}

	settings.IsInlineParams(value);
	RaisePropertyChanged(L"IsInlineParams");
}

bool HomeViewModel::IsSimulateExclusiveFullscreen() const noexcept {
	return AppSettings::Get().IsSimulateExclusiveFullscreen();
}

void HomeViewModel::IsSimulateExclusiveFullscreen(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsSimulateExclusiveFullscreen() == value) {
		return;
	}

	settings.IsSimulateExclusiveFullscreen(value);
	RaisePropertyChanged(L"IsSimulateExclusiveFullscreen");
}

static constexpr std::array MIN_FRAME_RATE_OPTIONS{ 0,5,10,20,30 };

IVector<IInspectable> HomeViewModel::MinFrameRateOptions() {
	static IVector<IInspectable> result = [] {
		std::vector<IInspectable> options;
		options.reserve(MIN_FRAME_RATE_OPTIONS.size());
		for (int option : MIN_FRAME_RATE_OPTIONS) {
			options.push_back(box_value(std::to_wstring(option)));
		}

		return single_threaded_vector(std::move(options));
	}();
	return result;
}

int HomeViewModel::MinFrameRateIndex() const noexcept {
	float minFrameRate = AppSettings::Get().MinFrameRate();
	auto it = std::find_if(
		MIN_FRAME_RATE_OPTIONS.begin(),
		MIN_FRAME_RATE_OPTIONS.end(),
		[&](int value) { return std::abs(minFrameRate - value) < 1e-5f; }
	);
	if (it == MIN_FRAME_RATE_OPTIONS.end()) {
		return -1;
	} else {
		return int(it - MIN_FRAME_RATE_OPTIONS.begin());
	}
}

void HomeViewModel::MinFrameRateIndex(int value) {
	if (value < 0 || value >= MIN_FRAME_RATE_OPTIONS.size()) {
		return;
	}

	AppSettings::Get().MinFrameRate((float)MIN_FRAME_RATE_OPTIONS[value]);
	RaisePropertyChanged(L"MinFrameRateIndex");
}

bool HomeViewModel::IsDeveloperMode() const noexcept {
	return AppSettings::Get().IsDeveloperMode();
}

void HomeViewModel::IsDeveloperMode(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsDeveloperMode() == value) {
		return;
	}

	settings.IsDeveloperMode(value);
	RaisePropertyChanged(L"IsDeveloperMode");
}

bool HomeViewModel::IsBenchmarkMode() const noexcept {
	return AppSettings::Get().IsBenchmarkMode();
}

void HomeViewModel::IsBenchmarkMode(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsBenchmarkMode() == value) {
		return;
	}

	settings.IsBenchmarkMode(value);
	RaisePropertyChanged(L"IsBenchmarkMode");
}

bool HomeViewModel::IsDebugMode() const noexcept {
	return AppSettings::Get().IsDebugMode();
}

void HomeViewModel::IsDebugMode(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsDebugMode() == value) {
		return;
	}

	settings.IsDebugMode(value);
	RaisePropertyChanged(L"IsDebugMode");
}

bool HomeViewModel::IsEffectCacheDisabled() const noexcept {
	return AppSettings::Get().IsEffectCacheDisabled();
}

void HomeViewModel::IsEffectCacheDisabled(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsEffectCacheDisabled() == value) {
		return;
	}

	settings.IsEffectCacheDisabled(value);
	RaisePropertyChanged(L"IsEffectCacheDisabled");
}

bool HomeViewModel::IsFontCacheDisabled() const noexcept {
	return AppSettings::Get().IsFontCacheDisabled();
}

void HomeViewModel::IsFontCacheDisabled(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsFontCacheDisabled() == value) {
		return;
	}

	settings.IsFontCacheDisabled(value);
	RaisePropertyChanged(L"IsFontCacheDisabled");
}

bool HomeViewModel::IsSaveEffectSources() const noexcept {
	return AppSettings::Get().IsSaveEffectSources();
}

void HomeViewModel::IsSaveEffectSources(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsSaveEffectSources() == value) {
		return;
	}

	settings.IsSaveEffectSources(value);
	RaisePropertyChanged(L"IsSaveEffectSources");
}

bool HomeViewModel::IsWarningsAreErrors() const noexcept {
	return AppSettings::Get().IsWarningsAreErrors();
}

void HomeViewModel::IsWarningsAreErrors(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsWarningsAreErrors() == value) {
		return;
	}

	settings.IsWarningsAreErrors(value);
	RaisePropertyChanged(L"IsWarningsAreErrors");
}

bool HomeViewModel::IsFP16Disabled() const noexcept {
	return AppSettings::Get().IsFP16Disabled();
}

void HomeViewModel::IsFP16Disabled(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsFP16Disabled() == value) {
		return;
	}

	settings.IsFP16Disabled(value);
	RaisePropertyChanged(L"IsFP16Disabled");
}

int HomeViewModel::DuplicateFrameDetectionMode() const noexcept {
	return (int)AppSettings::Get().DuplicateFrameDetectionMode();
}

void HomeViewModel::DuplicateFrameDetectionMode(int value) {
	if (value < 0) {
		return;
	}

	const auto mode = (::Magpie::DuplicateFrameDetectionMode)value;

	AppSettings& settings = AppSettings::Get();
	if (settings.DuplicateFrameDetectionMode() == mode) {
		return;
	}

	settings.DuplicateFrameDetectionMode(mode);

	RaisePropertyChanged(L"DuplicateFrameDetectionMode");
	RaisePropertyChanged(L"IsDynamicDection");

	if (mode != ::Magpie::DuplicateFrameDetectionMode::Dynamic) {
		settings.IsStatisticsForDynamicDetectionEnabled(false);
		RaisePropertyChanged(L"IsStatisticsForDynamicDetectionEnabled");
	}
}

bool HomeViewModel::IsDynamicDection() const noexcept {
	return AppSettings::Get().DuplicateFrameDetectionMode() == ::Magpie::DuplicateFrameDetectionMode::Dynamic;
}

bool HomeViewModel::IsStatisticsForDynamicDetectionEnabled() const noexcept {
	return AppSettings::Get().IsStatisticsForDynamicDetectionEnabled();
}

void HomeViewModel::IsStatisticsForDynamicDetectionEnabled(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsStatisticsForDynamicDetectionEnabled() == value) {
		return;
	}

	settings.IsStatisticsForDynamicDetectionEnabled(value);
	RaisePropertyChanged(L"IsStatisticsForDynamicDetectionEnabled");
}

void HomeViewModel::_ScalingService_IsTimerOnChanged(bool value) {
	if (!value) {
		RaisePropertyChanged(L"TimerProgressRingValue");
	}

	RaisePropertyChanged(L"TimerProgressRingValue");
	RaisePropertyChanged(L"TimerLabelText");
	RaisePropertyChanged(L"TimerButtonText");
	RaisePropertyChanged(L"IsTimerOn");
}

void HomeViewModel::_ScalingService_TimerTick(double) {
	RaisePropertyChanged(L"TimerProgressRingValue");
	RaisePropertyChanged(L"TimerLabelText");
}

void HomeViewModel::_ScalingService_IsRunningChanged(bool) {
	RaisePropertyChanged(L"IsNotRunning");
}

}
