#include "pch.h"
#include "MainPage.h"
#include "SettingsPage.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif

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

}
