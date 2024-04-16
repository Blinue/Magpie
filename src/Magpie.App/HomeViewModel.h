#pragma once
#include "HomeViewModel.g.h"
#include "WinRTUtils.h"

namespace winrt::Magpie::App::implementation {

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

	bool IsAutoRestore() const noexcept;
	void IsAutoRestore(bool value);

	bool IsWndToRestore() const noexcept;

	void ActivateRestore() const noexcept;

	void ClearRestore() const;

	hstring RestoreWndDesc() const noexcept;

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
private:
	void _ScalingService_IsTimerOnChanged(bool value);

	void _ScalingService_TimerTick(double);

	void _ScalingService_IsRunningChanged(bool);

	void _ScalingService_WndToRestoreChanged(HWND);

	WinRTUtils::EventRevoker _isTimerOnRevoker;
	WinRTUtils::EventRevoker _timerTickRevoker;
	WinRTUtils::EventRevoker _isRunningChangedRevoker;
	WinRTUtils::EventRevoker _wndToRestoreChangedRevoker;
	WinRTUtils::EventRevoker _isShowOnHomePageChangedRevoker;

	bool _showUpdateCard = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HomeViewModel : HomeViewModelT<HomeViewModel, implementation::HomeViewModel> {
};

}
