#pragma once
#include "AboutViewModel.g.h"

namespace winrt::Magpie::UI::implementation {

struct AboutViewModel : AboutViewModelT<AboutViewModel> {
    AboutViewModel() = default;

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

	bool IsCheckingForUpdates() const noexcept {
		return _isCheckingForUpdates;
	}

	bool IsNetworkError() const noexcept;
	bool IsUnknownError() const noexcept;
	bool IsNoUpdate() const noexcept;
	bool IsAvailable() const noexcept;

	Uri UpdateReleaseNotesLink() const noexcept;

private:
	event<PropertyChangedEventHandler> _propertyChangedEvent;

	bool _isCheckingForUpdates = false;
	// -1 表示尚未检查更新，否则为 UpdateResult
	int _updateStatus = -1;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct AboutViewModel : AboutViewModelT<AboutViewModel, implementation::AboutViewModel> {
};

}
