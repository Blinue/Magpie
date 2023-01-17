#pragma once
#include "AboutViewModel.g.h"
#include "UpdateService.h"

namespace winrt::Magpie::UI::implementation {

struct AboutViewModel : AboutViewModelT<AboutViewModel> {
	AboutViewModel();
	~AboutViewModel();

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

	bool IsNetworkErrorWhileChecking() const noexcept;
	void IsNetworkErrorWhileChecking(bool value) noexcept;

	bool IsOtherErrorWhileChecking() const noexcept;
	void IsOtherErrorWhileChecking(bool value) noexcept;

	bool IsNoUpdate() const noexcept;
	void IsNoUpdate(bool value) noexcept;

	bool IsAvailable() const noexcept;
	void IsAvailable(bool value) noexcept;

	Uri UpdateReleaseNotesLink() const noexcept;

private:
	void _UpdateService_StatusChanged(UpdateStatus);

	event<PropertyChangedEventHandler> _propertyChangedEvent;
	WinRTUtils::EventRevoker _updateStatusChangedRevoker;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct AboutViewModel : AboutViewModelT<AboutViewModel, implementation::AboutViewModel> {
};

}
