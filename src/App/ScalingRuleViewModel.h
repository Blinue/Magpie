#pragma once
#include "ScalingRuleViewModel.g.h"


namespace winrt::Magpie::App::implementation {

struct ScalingRuleViewModel : ScalingRuleViewModelT<ScalingRuleViewModel> {
	ScalingRuleViewModel() = default;

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

namespace winrt::Magpie::App::factory_implementation {

struct ScalingRuleViewModel : ScalingRuleViewModelT<ScalingRuleViewModel, implementation::ScalingRuleViewModel> {
};

}
