#pragma once

#include "Settings.h"
#include "SettingsPage.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsPage : SettingsPageT<SettingsPage> {
	SettingsPage();

	void ThemeComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&);

	void PortableModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void ComboBox_DropDownOpened(IInspectable const&, IInspectable const&);

	void BreakpointModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void DisableEffectCacheToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void SaveEffectSourcesToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void WarningsAreErrorsToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

private:
	Magpie::App::Settings _settings{ nullptr };
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsPage : SettingsPageT<SettingsPage, implementation::SettingsPage> {
};

}
