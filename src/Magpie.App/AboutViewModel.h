#pragma once
#include "AboutViewModel.g.h"
#include "UpdateService.h"

namespace winrt::Magpie::App::implementation {

struct AboutViewModel : AboutViewModelT<AboutViewModel> {
	AboutViewModel();

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	hstring Version() const noexcept;

	Uri ReleaseNotesLink() const noexcept;

	fire_and_forget CheckForUpdates();

	bool IsCheckForPreviewUpdates() const noexcept;
	void IsCheckForPreviewUpdates(bool value) noexcept;

	bool IsAutoCheckForUpdates() const noexcept;
	void IsAutoCheckForUpdates(bool value) noexcept;

	bool IsCheckingForUpdates() const noexcept;

	bool IsErrorWhileChecking() const noexcept;
	void IsErrorWhileChecking(bool value) noexcept;

	bool IsNoUpdate() const noexcept;
	void IsNoUpdate(bool value) noexcept;

	bool IsAvailable() const noexcept;

	bool IsDownloading() const noexcept;
	bool IsErrorWhileDownloading() const noexcept;
	bool IsDownloadingOrLater() const noexcept;
	bool IsInstalling() const noexcept;

	bool IsUpdateInfoBarOpen() const noexcept;
	void IsUpdateInfoBarOpen(bool value) noexcept;

	bool IsUpdateInfoBarClosable() const noexcept;
	bool IsCancelButtonVisible() const noexcept;

	bool IsNoDownloadProgress() const noexcept;
	double DownloadProgress() const noexcept;

	Uri UpdateReleaseNotesLink() const noexcept;

	fire_and_forget DownloadAndInstall();

	void Cancel();
	void Retry();

private:
	void _UpdateService_StatusChanged(UpdateStatus status);
	void _UpdateService_DownloadProgressChanged(double);

	event<PropertyChangedEventHandler> _propertyChangedEvent;
	WinRTUtils::EventRevoker _updateStatusChangedRevoker;
	WinRTUtils::EventRevoker _downloadProgressChangedRevoker;
	WinRTUtils::EventRevoker _showOnHomePageChangedRevoker;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct AboutViewModel : AboutViewModelT<AboutViewModel, implementation::AboutViewModel> {
};

}
