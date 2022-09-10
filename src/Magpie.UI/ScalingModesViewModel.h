#pragma once
#include "ScalingModesViewModel.g.h"


namespace winrt::Magpie::UI::implementation {

struct ScalingModesViewModel : ScalingModesViewModelT<ScalingModesViewModel> {
	ScalingModesViewModel();

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	IObservableVector<IInspectable> ScalingModes() const noexcept {
		return _scalingModes;
	}

private:
	event<PropertyChangedEventHandler> _propertyChangedEvent;

	IObservableVector<IInspectable> _scalingModes{ nullptr };
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingModesViewModel : ScalingModesViewModelT<ScalingModesViewModel, implementation::ScalingModesViewModel> {
};

}
