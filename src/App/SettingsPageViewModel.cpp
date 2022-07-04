#include "pch.h"
#include "SettingsPageViewModel.h"
#if __has_include("SettingsPageViewModel.g.cpp")
#include "SettingsPageViewModel.g.cpp"
#endif
#include "AppSettings.h"


namespace winrt::Magpie::App::implementation {

int32_t SettingsPageViewModel::Theme() const noexcept {
    return (int32_t)AppSettings::Get().Theme();
}

void SettingsPageViewModel::Theme(int32_t value) noexcept {
    if (value < 0) {
        return;
    }

    AppSettings& settings = AppSettings::Get();

    uint32_t theme = (uint32_t)value;
    if (settings.Theme() == theme) {
        return;
    }

    settings.Theme(theme);
    _propertyChangedEvent(*this, PropertyChangedEventArgs(L"Theme"));
}

bool SettingsPageViewModel::IsPortableMode() const noexcept {
    return (int32_t)AppSettings::Get().IsPortableMode();
}

void SettingsPageViewModel::IsPortableMode(bool value) noexcept {
    AppSettings& settings = AppSettings::Get();

    if (settings.IsPortableMode() == value) {
        return;
    }

    settings.IsPortableMode(value);
    _propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsPortableMode"));
}

bool SettingsPageViewModel::IsSimulateExclusiveFullscreen() const noexcept {
    return AppSettings::Get().IsSimulateExclusiveFullscreen();
}

void SettingsPageViewModel::IsSimulateExclusiveFullscreen(bool value) noexcept {
    AppSettings& settings = AppSettings::Get();

    if (settings.IsSimulateExclusiveFullscreen() == value) {
        return;
    }

    settings.IsSimulateExclusiveFullscreen(value);
    _propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsSimulateExclusiveFullscreen"));
}

bool SettingsPageViewModel::IsBreakpointMode() const noexcept {
    return AppSettings::Get().IsBreakpointMode();
}

void SettingsPageViewModel::IsBreakpointMode(bool value) noexcept {
    AppSettings& settings = AppSettings::Get();

    if (settings.IsBreakpointMode() == value) {
        return;
    }

    settings.IsBreakpointMode(value);
    _propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsBreakpointMode"));
}

bool SettingsPageViewModel::IsDisableEffectCache() const noexcept {
    return AppSettings::Get().IsDisableEffectCache();
}

void SettingsPageViewModel::IsDisableEffectCache(bool value) noexcept {
    AppSettings& settings = AppSettings::Get();

    if (settings.IsDisableEffectCache() == value) {
        return;
    }

    settings.IsDisableEffectCache(value);
    _propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDisableEffectCache"));
}

bool SettingsPageViewModel::IsSaveEffectSources() const noexcept {
    return AppSettings::Get().IsSaveEffectSources();
}

void SettingsPageViewModel::IsSaveEffectSources(bool value) noexcept {
    AppSettings& settings = AppSettings::Get();

    if (settings.IsSaveEffectSources() == value) {
        return;
    }

    settings.IsSaveEffectSources(value);
    _propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsSaveEffectSources"));
}

bool SettingsPageViewModel::IsWarningsAreErrors() const noexcept {
    return AppSettings::Get().IsWarningsAreErrors();
}

void SettingsPageViewModel::IsWarningsAreErrors(bool value) noexcept {
    AppSettings& settings = AppSettings::Get();

    if (settings.IsWarningsAreErrors() == value) {
        return;
    }

    settings.IsWarningsAreErrors(value);
    _propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsWarningsAreErrors"));
}

}
