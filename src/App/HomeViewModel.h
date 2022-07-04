#pragma once
#include "HomeViewModel.g.h"


namespace winrt::Magpie::App::implementation {

struct HomeViewModel : HomeViewModelT<HomeViewModel> {
    HomeViewModel() = default;

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

struct HomeViewModel : HomeViewModelT<HomeViewModel, implementation::HomeViewModel> {
};

}
