#include "pch.h"
#include "AboutViewModel.h"
#if __has_include("AboutViewModel.g.cpp")
#include "AboutViewModel.g.cpp"
#endif
#include "Version.h"
#include "UpdateService.h"
#include "AppSettings.h"

namespace winrt::Magpie::UI::implementation {

AboutViewModel::AboutViewModel() {
	_updateStatusChangedRevoker = UpdateService::Get().StatusChanged(
		auto_revoke, { this, &AboutViewModel::_UpdateService_StatusChanged });
}

AboutViewModel::~AboutViewModel() {
	UpdateService::Get().LeavingAboutPage();
}

hstring AboutViewModel::Version() const noexcept {
	return MAGPIE_TAG_W;
}

Uri AboutViewModel::ReleaseNotesLink() const noexcept {
	return Uri(StrUtils::ConcatW(L"https://github.com/Blinue/Magpie/releases/tag/", MAGPIE_TAG_W));
}

fire_and_forget AboutViewModel::CheckForUpdates() {
	return UpdateService::Get().CheckForUpdatesAsync();
}

bool AboutViewModel::IsCheckForPreviewUpdates() const noexcept {
	return AppSettings::Get().IsCheckForPreviewUpdates();
}

void AboutViewModel::IsCheckForPreviewUpdates(bool value) noexcept {
	AppSettings::Get().IsCheckForPreviewUpdates(value);
}

bool AboutViewModel::IsAutoCheckForUpdates() const noexcept {
	return AppSettings::Get().IsAutoCheckForUpdates();
}

void AboutViewModel::IsAutoCheckForUpdates(bool value) noexcept {
	AppSettings::Get().IsAutoCheckForUpdates(value);
}

bool AboutViewModel::IsCheckingForUpdates() const noexcept {
	return UpdateService::Get().Status() == UpdateStatus::Checking;
}

bool AboutViewModel::IsNetworkErrorWhileChecking() const noexcept {
	UpdateService& service = UpdateService::Get();
	return service.Status() == UpdateStatus::ErrorWhileChecking && service.Error() == UpdateError::Network;
}

void AboutViewModel::IsNetworkErrorWhileChecking(bool value) noexcept {
	if (!value) {
		UpdateService& service = UpdateService::Get();
		if (service.Status() == UpdateStatus::ErrorWhileChecking) {
			service.Cancel();
		}
	}
}

bool AboutViewModel::IsOtherErrorWhileChecking() const noexcept {
	UpdateService& service = UpdateService::Get();
	return service.Status() == UpdateStatus::ErrorWhileChecking && service.Error() != UpdateError::Network;
}

void AboutViewModel::IsOtherErrorWhileChecking(bool value) noexcept {
	IsNetworkErrorWhileChecking(value);
}

bool AboutViewModel::IsNoUpdate() const noexcept {
	return UpdateService::Get().Status() == UpdateStatus::NoUpdate;
}

void AboutViewModel::IsNoUpdate(bool value) noexcept {
	if (!value) {
		UpdateService& service = UpdateService::Get();
		if (service.Status() == UpdateStatus::NoUpdate) {
			service.Cancel();
		}
	}
}

bool AboutViewModel::IsAvailable() const noexcept {
	return UpdateService::Get().Status() == UpdateStatus::Available;
}

void AboutViewModel::IsAvailable(bool value) noexcept {
	if (!value) {
		UpdateService& service = UpdateService::Get();
		if (service.Status() == UpdateStatus::Available) {
			service.Cancel();
		}
	}
}

bool AboutViewModel::IsDownloading() const noexcept {
	return UpdateService::Get().Status() == UpdateStatus::Downloading;
}

Uri AboutViewModel::UpdateReleaseNotesLink() const noexcept {
	if (!IsAvailable()) {
		return nullptr;
	}

	return Uri(StrUtils::ConcatW(L"https://github.com/Blinue/Magpie/releases/tag/",
		UpdateService::Get().Tag()));
}

fire_and_forget AboutViewModel::DownloadAndInstall() {
	return UpdateService::Get().DownloadAndInstall();
}

void AboutViewModel::_UpdateService_StatusChanged(UpdateStatus) {
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsCheckingForUpdates"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsNetworkErrorWhileChecking"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsOtherErrorWhileChecking"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsNoUpdate"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsAvailable"));

	if (IsAvailable()) {
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"UpdateReleaseNotesLink"));
	}
}

}
