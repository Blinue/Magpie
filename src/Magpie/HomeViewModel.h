#pragma once
#include "HomeViewModel.g.h"
#include "Event.h"

namespace winrt::Magpie::implementation {

struct HomeViewModel : HomeViewModelT<HomeViewModel>, wil::notify_property_changed_base<HomeViewModel> {
	HomeViewModel();

	bool IsTimerOn() const noexcept;

	double TimerProgressRingValue() const noexcept;

	hstring TimerLabelText() const noexcept;

	hstring TimerButtonText() const noexcept;

	bool IsNotRunning() const noexcept;

	void ToggleTimer() const noexcept;

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

	bool IsTouchSupportEnabled() const noexcept;
	fire_and_forget IsTouchSupportEnabled(bool value);

	Uri TouchSupportLearnMoreUrl() const noexcept;

	bool IsShowTouchSupportInfoBar() const noexcept;

	bool IsAllowScalingMaximized() const noexcept;
	void IsAllowScalingMaximized(bool value);

	bool IsInlineParams() const noexcept;
	void IsInlineParams(bool value);

	bool IsSimulateExclusiveFullscreen() const noexcept;
	void IsSimulateExclusiveFullscreen(bool value);

	static IVector<IInspectable> MinFrameRateOptions();

	int MinFrameRateIndex() const noexcept;
	void MinFrameRateIndex(int value);

	bool IsDeveloperMode() const noexcept;
	void IsDeveloperMode(bool value);

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
	void _ScalingService_IsTimerOnChanged(bool value);

	void _ScalingService_TimerTick(double);

	void _ScalingService_IsRunningChanged(bool);

	::Magpie::Event<bool>::EventRevoker _isTimerOnRevoker;
	::Magpie::Event<double>::EventRevoker _timerTickRevoker;
	::Magpie::Event<bool>::EventRevoker _isRunningChangedRevoker;
	::Magpie::Event<bool>::EventRevoker _isShowOnHomePageChangedRevoker;

	bool _showUpdateCard = false;
};

}

namespace winrt::Magpie::factory_implementation {

struct HomeViewModel : HomeViewModelT<HomeViewModel, implementation::HomeViewModel> {};

}
