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
    void DownCount(uint32_t value);

    bool IsAutoRestore() const noexcept;
    void IsAutoRestore(bool value);

    bool IsWndToRestore() const noexcept;

    bool IsNoWndToRestore() const noexcept {
        return !IsWndToRestore();
    }

    void ActivateRestore() const noexcept;

    void ClearRestore() const;

private:
    void _MagService_IsCountingDownChanged(bool value);

    void _MagService_CountdownTick(float);

    void _MagService_IsRunningChanged(bool);

    void _MagService_WndToRestoreChanged(uint64_t);

    event<PropertyChangedEventHandler> _propertyChangedEvent;

    WinRTUtils::EventRevoker _isCountingDownRevoker;
    WinRTUtils::EventRevoker _countdownTickRevoker;
    WinRTUtils::EventRevoker _isRunningChangedRevoker;
    WinRTUtils::EventRevoker _wndToRestoreChangedRevoker;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HomeViewModel : HomeViewModelT<HomeViewModel, implementation::HomeViewModel> {
};

}
