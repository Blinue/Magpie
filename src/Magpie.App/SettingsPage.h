#pragma once

#include "MainPage.h"
#include "SettingsPage.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsPage : SettingsPageT<SettingsPage> {
    SettingsPage();

    void ThemeRadioButton_Checked(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::RoutedEventArgs const&);

    void Page_Loading(Windows::UI::Xaml::FrameworkElement const&, Windows::Foundation::IInspectable const&);

private:
    Magpie::App::MainPage _mainPage{ nullptr };
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsPage : SettingsPageT<SettingsPage, implementation::SettingsPage> {
};

}
