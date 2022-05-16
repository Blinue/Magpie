#include "pch.h"
#include "MainPage.h"
#include "SettingsPage.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::Magpie::App::implementation {

SettingsPage::SettingsPage() {
    InitializeComponent();
}

void SettingsPage::ThemeRadioButton_Checked(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const&) {
    uint8_t theme = 0;
    if (sender == LightThemeRadioButton()) {
        theme = 0;
    } else if (sender == DarkThemeRadioButton()) {
        theme = 1;
    } else {
        theme = 2;
    }

    _mainPage.Theme(theme);
}

void SettingsPage::Page_Loading(Windows::UI::Xaml::FrameworkElement const&, Windows::Foundation::IInspectable const&) {
    XamlRoot().Content().as(_mainPage);

    uint8_t theme = _mainPage.Theme();
    if (theme == 0) {
        LightThemeRadioButton().IsChecked(true);
    } else if (theme == 1) {
        DarkThemeRadioButton().IsChecked(true);
    } else {
        SystemThemeRadioButton().IsChecked(true);
    }
}
}
