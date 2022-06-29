#include "pch.h"
#include "SettingsPage.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif
#include "MainPage.h"
#include "XamlUtils.h"
#include "ComboBoxHelper.h"


using namespace winrt;
using namespace Windows::UI::Xaml::Input;


namespace winrt::Magpie::App::implementation {

SettingsPage::SettingsPage() {
	InitializeComponent();

	_settings = Application::Current().as<Magpie::App::App>().Settings();
	
	ThemeComboBox().SelectedIndex(_settings.Theme());
	PortableModeToggleSwitch().IsOn(_settings.IsPortableMode());
	SimulateExclusiveFullscreenToggleSwitch().IsOn(_settings.IsSimulateExclusiveFullscreen());
	BreakpointModeToggleSwitch().IsOn(_settings.IsBreakpointMode());
	DisableEffectCacheToggleSwitch().IsOn(_settings.IsDisableEffectCache());
	SaveEffectSourcesToggleSwitch().IsOn(_settings.IsSaveEffectSources());
	WarningsAreErrorsToggleSwitch().IsOn(_settings.IsWarningsAreErrors());
}

void SettingsPage::ThemeComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&) {
	_settings.Theme(ThemeComboBox().SelectedIndex());
}

void SettingsPage::PortableModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	_settings.IsPortableMode(PortableModeToggleSwitch().IsOn());
}

void SettingsPage::SimulateExclusiveFullscreenToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	_settings.IsSimulateExclusiveFullscreen(SimulateExclusiveFullscreenToggleSwitch().IsOn());
}

void SettingsPage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

void SettingsPage::BreakpointModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	_settings.IsBreakpointMode(BreakpointModeToggleSwitch().IsOn());
}

void SettingsPage::DisableEffectCacheToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	_settings.IsDisableEffectCache(DisableEffectCacheToggleSwitch().IsOn());
}

void SettingsPage::SaveEffectSourcesToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	_settings.IsSaveEffectSources(SaveEffectSourcesToggleSwitch().IsOn());
}

void SettingsPage::WarningsAreErrorsToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	_settings.IsWarningsAreErrors(WarningsAreErrorsToggleSwitch().IsOn());
}

}
