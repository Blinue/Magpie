#pragma once
#include "HomeViewModel.g.h"
#include "Event.h"

namespace winrt::Magpie::implementation {

struct HomeViewModel : HomeViewModelT<HomeViewModel>, wil::notify_property_changed_base<HomeViewModel> {
	HomeViewModel();

	hstring TimerDescription() const noexcept;

	bool IsTimerOn() const noexcept;

	double TimerProgressRingValue() const noexcept;

	hstring TimerLabelText() const noexcept;

	hstring TimerButtonText(bool windowedMode) const noexcept;

	hstring TimerFullscreenButtonText() const noexcept;

	hstring TimerWindowedButtonText() const noexcept;

	bool IsNotRunning() const noexcept;

	void ToggleTimerFullscreen() const noexcept;

	void ToggleTimerWindowed() const noexcept;

	uint32_t Delay() const noexcept;
	void Delay(uint32_t value);

	bool ShowUpdateCard() const noexcept {
		return _showUpdateCard;
	}

	void ShowUpdateCard(bool value) noexcept;

	hstring UpdateCardTitle() const noexcept;

	bool IsAutoCheckForUpdates() const noexcept;
	void IsAutoCheckForUpdates(bool value) noexcept;

	void DownloadAndInstall();

	void ReleaseNotes();

	void RemindMeLater();

	hstring InitialToolbarStateDescription() const noexcept;

	int FullscreenInitialToolbarState() const noexcept;
	void FullscreenInitialToolbarState(int value);

	int WindowedInitialToolbarState() const noexcept;
	void WindowedInitialToolbarState(int value);

	hstring ScreenshotSaveDirectory() const noexcept;

	void OpenScreenshotSaveDirectory() const noexcept;

	fire_and_forget ChangeScreenshotSaveDirectory() noexcept;

	bool IsTouchSupportEnabled() const noexcept;
	fire_and_forget IsTouchSupportEnabled(bool value);

	Uri TouchSupportLearnMoreUrl() const noexcept;

	bool IsShowTouchSupportInfoBar() const noexcept;

	bool IsAllowScalingMaximized() const noexcept;
	void IsAllowScalingMaximized(bool value);

	bool IsKeepOnTop() const noexcept;
	void IsKeepOnTop(bool value);

	bool IsSimulateExclusiveFullscreen() const noexcept;
	void IsSimulateExclusiveFullscreen(bool value);

	bool IsInlineParams() const noexcept;
	void IsInlineParams(bool value);

	static IVector<IInspectable> MinFrameRateOptions();

	int MinFrameRateIndex() const noexcept;
	void MinFrameRateIndex(int value);

	bool IsDeveloperMode() const noexcept;
	void IsDeveloperMode(bool value);

	void LocateMagpieLogs() noexcept;
	void LocateTouchHelperLogs() noexcept;
	void LocateUpdaterLogs() noexcept;

	bool IsBenchmarkMode() const noexcept;
	void IsBenchmarkMode(bool value);

	bool IsDebugMode() const noexcept;
	void IsDebugMode(bool value);

	bool IsEffectCacheDisabled() const noexcept;
	void IsEffectCacheDisabled(bool value);

	bool IsFontCacheDisabled() const noexcept;
	void IsFontCacheDisabled(bool value);

	bool IsSaveEffectSources() const noexcept;
	void IsSaveEffectSources(bool value);

	bool IsWarningsAreErrors() const noexcept;
	void IsWarningsAreErrors(bool value);

	bool IsFP16Disabled() const noexcept;
	void IsFP16Disabled(bool value);

	int DuplicateFrameDetectionMode() const noexcept;
	void DuplicateFrameDetectionMode(int value);

	bool IsDynamicDection() const noexcept;

	bool IsStatisticsForDynamicDetectionEnabled() const noexcept;
	void IsStatisticsForDynamicDetectionEnabled(bool value);

private:
	void _ScalingService_IsTimerOnChanged(bool value, bool windowedMode);

	void _ScalingService_TimerTick(double);

	void _ScalingService_IsScalingChanged(bool);

	void _ToggleTimer(bool windowedMode) const noexcept;

	::Magpie::Event<bool, bool>::EventRevoker _isTimerOnRevoker;
	::Magpie::Event<double>::EventRevoker _timerTickRevoker;
	::Magpie::Event<bool>::EventRevoker _isScalingChangedRevoker;
	::Magpie::Event<bool>::EventRevoker _isShowOnHomePageChangedRevoker;

	bool _showUpdateCard = false;
};

}

BASIC_FACTORY(HomeViewModel)
