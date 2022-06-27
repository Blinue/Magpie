#include "pch.h"
#include "SettingsPage.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif
#include "MainPage.h"
#include "XamlUtils.h"


using namespace winrt;
using namespace Windows::UI::Xaml::Input;


namespace winrt::Magpie::App::implementation {

SettingsPage::SettingsPage() {
	InitializeComponent();

	_settings = Application::Current().as<Magpie::App::App>().Settings();

	{
		bool isDeveloperMode = _settings.IsDeveloperMode();
		DeveloperModeToggleSwitch().IsOn(isDeveloperMode);

		_isDeveloperModeChangedRevoker = _settings.IsDeveloperModeChanged(
			auto_revoke,
			{ this, &SettingsPage::_Settings_IsDeveloperModeChanged }
		);
		_Settings_IsDeveloperModeChanged(nullptr, isDeveloperMode);
	}
	
	ThemeComboBox().SelectedIndex(_settings.Theme());
	PortableModeToggleSwitch().IsOn(_settings.IsPortableMode());
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

void SettingsPage::DeveloperModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	_settings.IsDeveloperMode(DeveloperModeToggleSwitch().IsOn());
}

void SettingsPage::ComboBox_DropDownOpened(IInspectable const&, IInspectable const&) {
	XamlUtils::UpdateThemeOfXamlPopups(XamlRoot(), ActualTheme());
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

void SettingsPage::_Settings_IsDeveloperModeChanged(IInspectable const&, bool value) {
	DebugSettingsGroup().Visibility(value ? Visibility::Visible : Visibility::Collapsed);
}

}
