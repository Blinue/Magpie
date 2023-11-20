#include "pch.h"
#include "HomeViewModel.h"
#if __has_include("HomeViewModel.g.cpp")
#include "HomeViewModel.g.cpp"
#endif
#include "AppSettings.h"
#include "MagService.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include "UpdateService.h"

namespace winrt::Magpie::App::implementation {

HomeViewModel::HomeViewModel() {
	MagService& magService = MagService::Get();

	_isRunningChangedRevoker = magService.IsRunningChanged(
		auto_revoke, { this, &HomeViewModel::_MagService_IsRunningChanged });
	_isTimerOnRevoker = magService.IsTimerOnChanged(
		auto_revoke, { this, &HomeViewModel::_MagService_IsTimerOnChanged });
	_timerTickRevoker = magService.TimerTick(
		auto_revoke, { this, &HomeViewModel::_MagService_TimerTick });
	_wndToRestoreChangedRevoker = magService.WndToRestoreChanged(
		auto_revoke, { this, &HomeViewModel::_MagService_WndToRestoreChanged });

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
	return MagService::Get().IsTimerOn();
}

double HomeViewModel::TimerProgressRingValue() const noexcept {
	MagService& magService = MagService::Get();
	return magService.IsTimerOn() ? magService.TimerProgress() : 1.0f;
}

hstring HomeViewModel::TimerLabelText() const noexcept {
	MagService& magService = MagService::Get();
	return to_hstring((int)std::ceil(magService.SecondsLeft()));
}

hstring HomeViewModel::TimerButtonText() const noexcept {
	MagService& magService = MagService::Get();
	ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView();
	if (magService.IsTimerOn()) {
		return resourceLoader.GetString(L"Home_Timer_Cancel");
	} else {
		hstring fmtStr = resourceLoader.GetString(L"Home_Timer_ButtonText");
		return hstring(fmt::format(
			fmt::runtime(std::wstring_view(fmtStr)),
			AppSettings::Get().CountdownSeconds()
		));
	}
}

bool HomeViewModel::IsNotRunning() const noexcept {
	return !MagService::Get().IsRunning();
}

void HomeViewModel::ToggleTimer() const noexcept {
	MagService& magService = MagService::Get();
	if (magService.IsTimerOn()) {
		magService.StopTimer();
	} else {
		magService.StartTimer();
	}
}

uint32_t HomeViewModel::Delay() const noexcept {
	return AppSettings::Get().CountdownSeconds();
}

void HomeViewModel::Delay(uint32_t value) {
	AppSettings::Get().CountdownSeconds(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Delay"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"TimerButtonText"));
}

bool HomeViewModel::IsAutoRestore() const noexcept {
	return AppSettings::Get().IsAutoRestore();
}

void HomeViewModel::IsAutoRestore(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsAutoRestore() == value) {
		return;
	}

	settings.IsAutoRestore(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsAutoRestore"));
}

bool HomeViewModel::IsWndToRestore() const noexcept {
	return (bool)MagService::Get().WndToRestore();
}

void HomeViewModel::ActivateRestore() const noexcept {
	HWND wndToRestore = (HWND)MagService::Get().WndToRestore();
	if (wndToRestore) {
		Win32Utils::SetForegroundWindow(wndToRestore);
	}
}

void HomeViewModel::ClearRestore() const {
	MagService::Get().ClearWndToRestore();
}

hstring HomeViewModel::RestoreWndDesc() const noexcept {
	HWND wndToRestore = (HWND)MagService::Get().WndToRestore();
	if (!wndToRestore) {
		return L"";
	}

	std::wstring title(GetWindowTextLength(wndToRestore), L'\0');
	GetWindowText(wndToRestore, title.data(), (int)title.size() + 1);

	ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView();
	hstring curWindow = resourceLoader.GetString(L"Home_AutoRestore_CurWindow");
	if (title.empty()) {
		hstring emptyTitle = resourceLoader.GetString(L"Home_AutoRestore_EmptyTitle");
		return hstring(StrUtils::Concat(curWindow, L"<", emptyTitle, L">"));
	} else {
		return curWindow + title;
	}
}

inline void HomeViewModel::ShowUpdateCard(bool value) noexcept {
	_showUpdateCard = value;
	if (!value) {
		UpdateService::Get().IsShowOnHomePage(false);
	}
	
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ShowUpdateCard"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"UpdateCardTitle"));
}

hstring HomeViewModel::UpdateCardTitle() const noexcept {
	UpdateService& updateService = UpdateService::Get();
	if (updateService.Status() < UpdateStatus::Available) {
		return {};
	}

	ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView();
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
	Application::Current().as<App>().RootPage().NavigateToAboutPage();
}

void HomeViewModel::ReleaseNotes() {
	std::wstring url = StrUtils::Concat(
		L"https://github.com/Blinue/Magpie/releases/tag/",
		UpdateService::Get().Tag()
	);
	Win32Utils::ShellOpen(url.c_str());
}

void HomeViewModel::RemindMeLater() {
	ShowUpdateCard(false);
}

void HomeViewModel::_MagService_IsTimerOnChanged(bool value) {
	if (!value) {
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"TimerProgressRingValue"));
	}

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"TimerProgressRingValue"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"TimerLabelText"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"TimerButtonText"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsTimerOn"));
}

void HomeViewModel::_MagService_TimerTick(double) {
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"TimerProgressRingValue"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"TimerLabelText"));
}

void HomeViewModel::_MagService_IsRunningChanged(bool) {
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsNotRunning"));
}

void HomeViewModel::_MagService_WndToRestoreChanged(HWND) {
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsWndToRestore"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"RestoreWndDesc"));
}

}
