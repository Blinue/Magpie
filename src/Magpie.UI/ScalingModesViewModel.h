#pragma once
#include "ScalingModesViewModel.g.h"


namespace winrt::Magpie::UI::implementation {

struct ScalingModesViewModel : ScalingModesViewModelT<ScalingModesViewModel> {
    ScalingModesViewModel() = default;

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

private:
	event<PropertyChangedEventHandler> _propertyChangedEvent;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingModesViewModel : ScalingModesViewModelT<ScalingModesViewModel, implementation::ScalingModesViewModel> {
};

}
