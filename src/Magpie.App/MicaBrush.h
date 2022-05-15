#pragma once
#include "pch.h"
#include "MicaBrush.g.h"


namespace winrt::Magpie::App::implementation {

struct MicaBrush : MicaBrushT<MicaBrush> {
    MicaBrush(Windows::UI::Xaml::FrameworkElement root);

    void OnHostFocusChanged(bool isFocused);

    void _UpdateBrush();

    void OnConnected();
    void OnDisconnected();

private:
    Windows::Foundation::IAsyncAction _Settings_ColorValuesChanged(
        Windows::UI::ViewManagement::UISettings const&,
        Windows::Foundation::IInspectable const&
    );
    Windows::Foundation::IAsyncAction _AccessibilitySettings_HighContrastChanged(
        Windows::UI::ViewManagement::AccessibilitySettings const&,
        Windows::Foundation::IInspectable const&
    );
    Windows::Foundation::IAsyncAction _CompositionCapabilities_Changed(
        Windows::UI::Composition::CompositionCapabilities sender,
        Windows::Foundation::IInspectable const&
    );
    Windows::Foundation::IAsyncAction _PowerManager_EnergySaverStatusChanged(
        Windows::Foundation::IInspectable const&,
        Windows::Foundation::IInspectable const&
    );
    void _RootElement_ActualThemeChanged(
        Windows::UI::Xaml::FrameworkElement const&,
        Windows::Foundation::IInspectable const&
    );

    Windows::UI::Xaml::FrameworkElement _rootElement{ nullptr };
    bool _windowActivated = false;

    Windows::UI::ViewManagement::UISettings _settings{ nullptr };
    Windows::UI::ViewManagement::AccessibilitySettings _accessibilitySettings{ nullptr };
    std::optional<bool> _fastEffects;
    std::optional<bool> _energySaver;

    winrt::event_token _colorValuesChangedToken{};
    winrt::event_token _highContrastChangedToken{};
    winrt::event_token _energySaverStatusChangedToken{};
    winrt::event_token _compositionCapabilitiesChangedToken{};
    winrt::event_token _rootElementThemeChangedToken{};
};

}

namespace winrt::Magpie::App::factory_implementation {

struct MicaBrush : MicaBrushT<MicaBrush, implementation::MicaBrush> {
};

}
