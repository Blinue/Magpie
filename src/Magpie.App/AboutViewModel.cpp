#include "pch.h"
#include "AboutViewModel.h"
#if __has_include("AboutViewModel.g.cpp")
#include "AboutViewModel.g.cpp"
#endif
#include "Version.h"
#include "UpdateService.h"
#include "AppSettings.h"

namespace winrt::Magpie::App::implementation {

AboutViewModel::AboutViewModel() {
	UpdateService& service = UpdateService::Get();
	_updateStatusChangedRevoker = service.StatusChanged(
		auto_revoke, { this, &AboutViewModel::_UpdateService_StatusChanged });
	
	if (service.Status() == UpdateStatus::Downloading) {
		_downloadProgressChangedRevoker = UpdateService::Get().DownloadProgressChanged(
			auto_revoke,
			{ this, &AboutViewModel::_UpdateService_DownloadProgressChanged }
		);
	}

	// 用户查看了关于页面，主页无需再显示更新提示
	service.IsShowOnHomePage(false);
	_showOnHomePageChangedRevoker = service.IsShowOnHomePageChanged(auto_revoke, [](bool value) {
		if (value) {
			// 在关于页面触发自动更新时主页不需要显示更新提示
			UpdateService::Get().IsShowOnHomePage(false);
		}
	});
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
	return UpdateService::Get().CheckForUpdatesAsync(false);
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

bool AboutViewModel::IsErrorWhileChecking() const noexcept {
	UpdateService& service = UpdateService::Get();
	return service.Status() == UpdateStatus::ErrorWhileChecking;
}

void AboutViewModel::IsErrorWhileChecking(bool value) noexcept {
	if (!value) {
		UpdateService& service = UpdateService::Get();
		if (service.Status() == UpdateStatus::ErrorWhileChecking) {
			service.Cancel();
		}
	}
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

bool AboutViewModel::IsDownloading() const noexcept {
	return UpdateService::Get().Status() == UpdateStatus::Downloading;
}

bool AboutViewModel::IsErrorWhileDownloading() const noexcept {
	return UpdateService::Get().Status() == UpdateStatus::ErrorWhileDownloading;
}

bool AboutViewModel::IsDownloadingOrLater() const noexcept {
	return UpdateService::Get().Status() >= UpdateStatus::Downloading;
}

bool AboutViewModel::IsInstalling() const noexcept {
	return UpdateService::Get().Status() == UpdateStatus::Installing;
}

bool AboutViewModel::IsUpdateInfoBarOpen() const noexcept {
	return UpdateService::Get().Status() >= UpdateStatus::Available;
}

void AboutViewModel::IsUpdateInfoBarOpen(bool value) noexcept {
	if (!value) {
		UpdateService& service = UpdateService::Get();
		UpdateStatus status = service.Status();
		if (status == UpdateStatus::Available || status == UpdateStatus::ErrorWhileDownloading) {
			service.Cancel();
		}
	}
}

bool AboutViewModel::IsUpdateInfoBarClosable() const noexcept {
	UpdateStatus status = UpdateService::Get().Status();
	return status == UpdateStatus::Available || status == UpdateStatus::ErrorWhileDownloading;
}

bool AboutViewModel::IsCancelButtonVisible() const noexcept {
	UpdateStatus status = UpdateService::Get().Status();
	return status == UpdateStatus::Downloading || status == UpdateStatus::Installing;
}

bool AboutViewModel::IsNoDownloadProgress() const noexcept {
	UpdateService& service = UpdateService::Get();
	switch (service.Status()) {
	case UpdateStatus::Downloading:
		return service.DownloadProgress() < 1e-6;
	default:
		return true;
	}
}

double AboutViewModel::DownloadProgress() const noexcept {
	switch (UpdateService::Get().Status()) {
	case UpdateStatus::Downloading:
		return UpdateService::Get().DownloadProgress();
	case UpdateStatus::Installing:
		return 1.0;
	default:
		return 0.0;
	}
}

Uri AboutViewModel::UpdateReleaseNotesLink() const noexcept {
	if (!IsUpdateInfoBarOpen()) {
		return nullptr;
	}

	return Uri(StrUtils::ConcatW(L"https://github.com/Blinue/Magpie/releases/tag/",
		UpdateService::Get().Tag()));
}

fire_and_forget AboutViewModel::DownloadAndInstall() {
	return UpdateService::Get().DownloadAndInstall();
}

void AboutViewModel::Cancel() {
	assert(UpdateService::Get().Status() == UpdateStatus::Downloading);
	UpdateService::Get().Cancel();
}

void AboutViewModel::Retry() {
	assert(UpdateService::Get().Status() == UpdateStatus::ErrorWhileDownloading);
	UpdateService::Get().DownloadAndInstall();
}

void AboutViewModel::_UpdateService_StatusChanged(UpdateStatus status) {
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsCheckingForUpdates"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsErrorWhileChecking"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsNoUpdate"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsAvailable"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDownloading"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsErrorWhileDownloading"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsInstalling"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDownloadingOrLater"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsUpdateInfoBarOpen"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsUpdateInfoBarClosable"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsCancelButtonVisible"));

	if (status >= UpdateStatus::Available) {
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"UpdateReleaseNotesLink"));

		if (status == UpdateStatus::Downloading) {
			_downloadProgressChangedRevoker = UpdateService::Get().DownloadProgressChanged(
				auto_revoke,
				{ this, &AboutViewModel::_UpdateService_DownloadProgressChanged }
			);
		} else {
			_downloadProgressChangedRevoker.Revoke();

			if (status >= UpdateStatus::ErrorWhileDownloading) {
				_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsNoDownloadProgress"));
			}
		}
	}
}

void AboutViewModel::_UpdateService_DownloadProgressChanged(double) {
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsNoDownloadProgress"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"DownloadProgress"));
}

}
