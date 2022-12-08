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

	fire_and_forget CheckForUpdate();

	bool IsCheckForPreviewUpdates() const noexcept;
	void IsCheckForPreviewUpdates(bool value) noexcept;

	bool IsAutoDownloadUpdates() const noexcept;
	void IsAutoDownloadUpdates(bool value) noexcept;

private:
	event<PropertyChangedEventHandler> _propertyChangedEvent;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct AboutViewModel : AboutViewModelT<AboutViewModel, implementation::AboutViewModel> {
};

}
