#pragma once
#include "AboutViewModel.g.h"
#include "UpdateService.h"

namespace winrt::Magpie::implementation {

struct AboutViewModel : AboutViewModelT<AboutViewModel>,
                        wil::notify_property_changed_base<AboutViewModel> {
	AboutViewModel();

	Imaging::SoftwareBitmapSource Logo() const noexcept {
		return _logo;
	}

	hstring Version() const noexcept;

	bool IsDeveloperMode() const noexcept;
	void IsDeveloperMode(bool value);

	fire_and_forget CheckForUpdates();

	bool IsCheckForPreviewUpdates() const noexcept;
	void IsCheckForPreviewUpdates(bool value);

	bool IsCheckForUpdatesButtonEnabled() const noexcept;

	bool IsAutoCheckForUpdates() const noexcept;
	void IsAutoCheckForUpdates(bool value);

	bool IsAnyUpdateStatus() const noexcept;

	bool IsCheckingForUpdates() const noexcept;

	bool IsErrorWhileChecking() const noexcept;
	void IsErrorWhileChecking(bool value);

	bool IsNoUpdate() const noexcept;
	void IsNoUpdate(bool value) const noexcept;

	bool IsAvailable() const noexcept;

	bool IsDownloading() const noexcept;
	bool IsErrorWhileDownloading() const noexcept;
	bool IsDownloadingOrLater() const noexcept;
	bool IsInstalling() const noexcept;

	bool IsUpdateCardOpen() const noexcept;
	void IsUpdateCardOpen(bool value);

	bool IsUpdateCardClosable() const noexcept;
	bool IsCancelButtonVisible() const noexcept;

	hstring UpdateCardTitle() const noexcept;

	bool IsNoDownloadProgress() const noexcept;
	double DownloadProgress() const noexcept;

	Uri UpdateReleaseNotesLink() const noexcept;

	fire_and_forget DownloadAndInstall();

	void Cancel();
	void Retry();

private:
	void _UpdateService_StatusChanged(UpdateStatus status);
	void _UpdateService_DownloadProgressChanged(double);

	WinRTHelper::EventRevoker _updateStatusChangedRevoker;
	WinRTHelper::EventRevoker _downloadProgressChangedRevoker;
	WinRTHelper::EventRevoker _showOnHomePageChangedRevoker;

	Imaging::SoftwareBitmapSource _logo{ nullptr };
};

}

namespace winrt::Magpie::factory_implementation {

struct AboutViewModel : AboutViewModelT<AboutViewModel, implementation::AboutViewModel> {
};

}
