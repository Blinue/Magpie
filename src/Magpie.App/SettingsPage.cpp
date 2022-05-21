#include "pch.h"
#include "SettingsPage.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif
#include "MainPage.h"
#include "Utils.h"


using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;


namespace winrt::Magpie::App::implementation {

SettingsPage::SettingsPage() {
	InitializeComponent();
}

void SettingsPage::Page_Loading(Windows::UI::Xaml::FrameworkElement const&, Windows::Foundation::IInspectable const&) {
	XamlRoot().Content().as(_mainPage);

	ThemeComboBox().SelectedIndex(_mainPage.Theme());
}

void SettingsPage::ThemeComboBox_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&) {
	_mainPage.Theme((uint8_t)ThemeComboBox().SelectedIndex());
}

void SettingsPage::ComboBox_DropDownOpened(IInspectable const&, IInspectable const&) {
	Utils::UpdateThemeOfXamlPopups(XamlRoot(), _mainPage.ActualTheme());
}

}
