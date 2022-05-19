#pragma once

#include "MainPage.h"
#include "SettingsPage.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsPage : SettingsPageT<SettingsPage> {
	SettingsPage();

	void Page_Loading(Windows::UI::Xaml::FrameworkElement const&, Windows::Foundation::IInspectable const&);

	void ThemeComboBox_SelectionChanged(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

private:
	Magpie::App::MainPage _mainPage{ nullptr };
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsPage : SettingsPageT<SettingsPage, implementation::SettingsPage> {
};

}
