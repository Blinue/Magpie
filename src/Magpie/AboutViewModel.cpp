#include "pch.h"
#include "AboutViewModel.h"
#if __has_include("AboutViewModel.g.cpp")
#include "AboutViewModel.g.cpp"
#endif
#include "Version.h"
#include "UpdateService.h"
#include "AppSettings.h"
#include "StrHelper.h"
#include "IconHelper.h"
#include "CommonSharedConstants.h"

using namespace ::Magpie;
using namespace ::Magpie::Core;
using namespace winrt;
using namespace Windows::UI::Xaml::Media::Imaging;

namespace winrt::Magpie::implementation {

AboutViewModel::AboutViewModel() {
	UpdateService& service = UpdateService::Get();
	service.EnteringAboutPage();

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

	// 异步加载 Logo
	([](AboutViewModel* that)->fire_and_forget {
		auto weakThis = that->get_weak();
		SoftwareBitmapSource bitmap;
		co_await bitmap.SetBitmapAsync(IconHelper::ExtractAppIcon(256));

		if (!weakThis.get()) {
			co_return;
		}

		that->_logo = std::move(bitmap);
		that->RaisePropertyChanged(L"Logo");
	})(this);
}

hstring AboutViewModel::Version() const noexcept {
	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	return hstring(StrHelper::Concat(
		resourceLoader.GetString(L"About_Version_Version"),
#ifdef MAGPIE_VERSION_TAG
		L" ",
		WIDEN(STRING(MAGPIE_VERSION_TAG)) + 1,
#else
		L" dev",
#endif
#ifdef MAGPIE_COMMIT_ID
		L" | ",
		resourceLoader.GetString(L"About_Version_CommitId"),
		L" " WIDEN(STRING(MAGPIE_COMMIT_ID)),
#endif
		L" | "
#ifdef _M_X64
		L"x64"
#elif defined(_M_ARM64)
		L"ARM64"
#else
		static_assert(false, "不支持的架构")
#endif
	));
}

bool AboutViewModel::IsDeveloperMode() const noexcept {
	return AppSettings::Get().IsDeveloperMode();
}

void AboutViewModel::IsDeveloperMode(bool value) {
	AppSettings::Get().IsDeveloperMode(value);
}

fire_and_forget AboutViewModel::CheckForUpdates() {
	return UpdateService::Get().CheckForUpdatesAsync(false);
}

bool AboutViewModel::IsCheckForPreviewUpdates() const noexcept {
	return AppSettings::Get().IsCheckForPreviewUpdates();
}

void AboutViewModel::IsCheckForPreviewUpdates(bool value) {
	AppSettings::Get().IsCheckForPreviewUpdates(value);
	RaisePropertyChanged(L"IsCheckForPreviewUpdates");
}

bool AboutViewModel::IsCheckForUpdatesButtonEnabled() const noexcept {
#ifndef MAGPIE_VERSION_TAG
	// 只有正式版本才能检查更新
	return false;
#endif

	return !IsCheckingForUpdates() && !IsDownloadingOrLater();
}

bool AboutViewModel::IsAutoCheckForUpdates() const noexcept {
	return AppSettings::Get().IsAutoCheckForUpdates();
}

void AboutViewModel::IsAutoCheckForUpdates(bool value) {
	AppSettings::Get().IsAutoCheckForUpdates(value);
	RaisePropertyChanged(L"IsAutoCheckForUpdates");
}

bool AboutViewModel::IsAnyUpdateStatus() const noexcept {
	return UpdateService::Get().Status() > UpdateStatus::Checking;
}

bool AboutViewModel::IsCheckingForUpdates() const noexcept {
	return UpdateService::Get().Status() == UpdateStatus::Checking;
}

bool AboutViewModel::IsErrorWhileChecking() const noexcept {
	return UpdateService::Get().Status() == UpdateStatus::ErrorWhileChecking;
}

void AboutViewModel::IsErrorWhileChecking(bool value) {
	if (!value) {
		UpdateService& service = UpdateService::Get();
		if (service.Status() == UpdateStatus::ErrorWhileChecking) {
			service.Cancel();
		}
	}

	RaisePropertyChanged(L"IsErrorWhileChecking");
}

bool AboutViewModel::IsNoUpdate() const noexcept {
	return UpdateService::Get().Status() == UpdateStatus::NoUpdate;
}

void AboutViewModel::IsNoUpdate(bool value) const noexcept {
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

bool AboutViewModel::IsUpdateCardOpen() const noexcept {
	return UpdateService::Get().Status() >= UpdateStatus::Available;
}

void AboutViewModel::IsUpdateCardOpen(bool value) {
	if (!value) {
		UpdateService& service = UpdateService::Get();
		UpdateStatus status = service.Status();
		if (status == UpdateStatus::Available || status == UpdateStatus::ErrorWhileDownloading) {
			service.Cancel();
		}
	}

	RaisePropertyChanged(L"IsUpdateCardOpen");
}

bool AboutViewModel::IsUpdateCardClosable() const noexcept {
	UpdateStatus status = UpdateService::Get().Status();
	return status == UpdateStatus::Available || status == UpdateStatus::ErrorWhileDownloading;
}

bool AboutViewModel::IsCancelButtonVisible() const noexcept {
	UpdateStatus status = UpdateService::Get().Status();
	return status == UpdateStatus::Downloading || status == UpdateStatus::Installing;
}

hstring AboutViewModel::UpdateCardTitle() const noexcept {
	UpdateService& updateService = UpdateService::Get();
	if (updateService.Status() < UpdateStatus::Available) {
		return {};
	}

	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	hstring titleFmt = resourceLoader.GetString(L"Home_UpdateCard_Title");
	return hstring(fmt::format(fmt::runtime(std::wstring_view(titleFmt)), updateService.Tag()));
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
	if (!IsUpdateCardOpen()) {
		return nullptr;
	}

	return Uri(StrHelper::Concat(L"https://github.com/Blinue/Magpie/releases/tag/",
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
	RaisePropertyChanged(L"IsCheckingForUpdates");
	RaisePropertyChanged(L"IsCheckForUpdatesButtonEnabled");
	RaisePropertyChanged(L"IsAnyUpdateStatus");
	RaisePropertyChanged(L"IsErrorWhileChecking");
	RaisePropertyChanged(L"IsNoUpdate");
	RaisePropertyChanged(L"IsAvailable");
	RaisePropertyChanged(L"IsDownloading");
	RaisePropertyChanged(L"IsErrorWhileDownloading");
	RaisePropertyChanged(L"IsInstalling");
	RaisePropertyChanged(L"IsDownloadingOrLater");
	RaisePropertyChanged(L"IsUpdateCardOpen");
	RaisePropertyChanged(L"IsUpdateCardClosable");
	RaisePropertyChanged(L"IsCancelButtonVisible");

	if (status >= UpdateStatus::Available) {
		RaisePropertyChanged(L"UpdateCardTitle");
		RaisePropertyChanged(L"UpdateReleaseNotesLink");

		if (status == UpdateStatus::Downloading) {
			_downloadProgressChangedRevoker = UpdateService::Get().DownloadProgressChanged(
				auto_revoke,
				{ this, &AboutViewModel::_UpdateService_DownloadProgressChanged }
			);
		} else {
			_downloadProgressChangedRevoker.Revoke();

			if (status >= UpdateStatus::ErrorWhileDownloading) {
				RaisePropertyChanged(L"IsNoDownloadProgress");
			}
		}
	}
}

void AboutViewModel::_UpdateService_DownloadProgressChanged(double) {
	RaisePropertyChanged(L"IsNoDownloadProgress");
	RaisePropertyChanged(L"DownloadProgress");
}

}
