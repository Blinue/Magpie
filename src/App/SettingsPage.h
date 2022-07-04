#pragma once
#include "SettingsPage.g.h"


namespace winrt::Magpie::App::implementation {

struct SettingsPage : SettingsPageT<SettingsPage> {
	SettingsPage();

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

	void ThemeComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&);

	void PortableModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void SimulateExclusiveFullscreenToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void BreakpointModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void DisableEffectCacheToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void SaveEffectSourcesToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void WarningsAreErrorsToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsPage : SettingsPageT<SettingsPage, implementation::SettingsPage> {
};

}
