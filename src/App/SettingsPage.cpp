#include "pch.h"
#include "SettingsPage.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif
#include "MainPage.h"
#include "XamlUtils.h"
#include "ComboBoxHelper.h"
#include "AppSettings.h"


using namespace winrt;
using namespace Windows::UI::Xaml::Input;


namespace winrt::Magpie::App::implementation {

SettingsPage::SettingsPage() {
	InitializeComponent();

	AppSettings& settings = AppSettings::Get();
	
	ThemeComboBox().SelectedIndex(settings.Theme());
	PortableModeToggleSwitch().IsOn(settings.IsPortableMode());
	SimulateExclusiveFullscreenToggleSwitch().IsOn(settings.IsSimulateExclusiveFullscreen());
	BreakpointModeToggleSwitch().IsOn(settings.IsBreakpointMode());
	DisableEffectCacheToggleSwitch().IsOn(settings.IsDisableEffectCache());
	SaveEffectSourcesToggleSwitch().IsOn(settings.IsSaveEffectSources());
	WarningsAreErrorsToggleSwitch().IsOn(settings.IsWarningsAreErrors());
}

void SettingsPage::ThemeComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&) {
	AppSettings::Get().Theme(ThemeComboBox().SelectedIndex());
}

void SettingsPage::PortableModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	AppSettings::Get().IsPortableMode(PortableModeToggleSwitch().IsOn());
}

void SettingsPage::SimulateExclusiveFullscreenToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	AppSettings::Get().IsSimulateExclusiveFullscreen(SimulateExclusiveFullscreenToggleSwitch().IsOn());
}

void SettingsPage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

void SettingsPage::BreakpointModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	AppSettings::Get().IsBreakpointMode(BreakpointModeToggleSwitch().IsOn());
}

void SettingsPage::DisableEffectCacheToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	AppSettings::Get().IsDisableEffectCache(DisableEffectCacheToggleSwitch().IsOn());
}

void SettingsPage::SaveEffectSourcesToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	AppSettings::Get().IsSaveEffectSources(SaveEffectSourcesToggleSwitch().IsOn());
}

void SettingsPage::WarningsAreErrorsToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	AppSettings::Get().IsWarningsAreErrors(WarningsAreErrorsToggleSwitch().IsOn());
}

}
