#pragma once
#include "SettingsViewModel.g.h"


namespace winrt::Magpie::App::implementation {

struct SettingsViewModel : SettingsViewModelT<SettingsViewModel> {
    SettingsViewModel() = default;

    int32_t Theme() const noexcept;
    void Theme(int32_t value) noexcept;

    bool IsPortableMode() const noexcept;
    void IsPortableMode(bool value) noexcept;

    bool IsSimulateExclusiveFullscreen() const noexcept;
    void IsSimulateExclusiveFullscreen(bool value) noexcept;

    bool IsBreakpointMode() const noexcept;
    void IsBreakpointMode(bool value) noexcept;

    bool IsDisableEffectCache() const noexcept;
    void IsDisableEffectCache(bool value) noexcept;

    bool IsSaveEffectSources() const noexcept;
    void IsSaveEffectSources(bool value) noexcept;

    bool IsWarningsAreErrors() const noexcept;
    void IsWarningsAreErrors(bool value) noexcept;

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

struct SettingsViewModel : SettingsViewModelT<SettingsViewModel, implementation::SettingsViewModel> {
};

}
