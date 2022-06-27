#pragma once

#include "Settings.h"
#include "SettingsPage.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsPage : SettingsPageT<SettingsPage> {
	SettingsPage();

	void ThemeComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&);

	void PortableModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void DeveloperModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void ComboBox_DropDownOpened(IInspectable const&, IInspectable const&);

private:
	void _Settings_IsDeveloperModeChanged(IInspectable const&, bool value);

	Magpie::App::Settings _settings{ nullptr };

	Magpie::App::Settings::IsDeveloperModeChanged_revoker _isDeveloperModeChangedRevoker;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsPage : SettingsPageT<SettingsPage, implementation::SettingsPage> {
};

}
