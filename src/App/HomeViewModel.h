#pragma once
#include "HomeViewModel.g.h"
#include "WinRTUtils.h"


namespace winrt::Magpie::App::implementation {

struct HomeViewModel : HomeViewModelT<HomeViewModel> {
    HomeViewModel();

    event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
        return _propertyChangedEvent.add(handler);
    }

    void PropertyChanged(event_token const& token) noexcept {
        _propertyChangedEvent.remove(token);
    }

    bool IsCountingDown() const noexcept;

    float CountdownProgressRingValue() const noexcept;

    hstring CountdownLabelText() const noexcept;

    hstring CountdownButtonText() const noexcept;

    bool IsNotRunning() const noexcept;

    void ToggleCountdown() const noexcept;

    uint32_t DownCount() const noexcept;
    void DownCount(uint32_t value) noexcept;

private:
    void _MagService_IsCountingDownChanged(bool value);

    void _MagService_CountdownTick(float);

    void _MagService_IsRunningChanged(bool value);

    event<PropertyChangedEventHandler> _propertyChangedEvent;

    WinRTUtils::EventRevoker _isCountingDownRevoker;
    WinRTUtils::EventRevoker _countdownTickRevoker;
    WinRTUtils::EventRevoker _isRunningChangedRevoker;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HomeViewModel : HomeViewModelT<HomeViewModel, implementation::HomeViewModel> {
};

}
