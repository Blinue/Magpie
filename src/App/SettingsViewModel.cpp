#include "pch.h"
#include "SettingsViewModel.h"
#if __has_include("SettingsViewModel.g.cpp")
#include "SettingsViewModel.g.cpp"
#endif
#include "AppSettings.h"
#include "AutoStartHelper.h"
#include "Win32Utils.h"


namespace winrt::Magpie::App::implementation {

int32_t SettingsViewModel::Theme() const noexcept {
	return (int32_t)AppSettings::Get().Theme();
}

void SettingsViewModel::Theme(int32_t value) noexcept {
	if (value < 0) {
		return;
	}

	AppSettings& settings = AppSettings::Get();

	uint32_t theme = (uint32_t)value;
	if (settings.Theme() == theme) {
		return;
	}

	settings.Theme(theme);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Theme"));
}

bool SettingsViewModel::IsRunAtStartup() const noexcept {
	return AutoStartHelper::IsAutoStartTaskActive();
}

void SettingsViewModel::IsRunAtStartup(bool value) noexcept {
	if (value) {
		AutoStartHelper::CreateAutoStartTask(AppSettings::Get().IsAlwaysRunAsElevated());
	} else {
		AutoStartHelper::DeleteAutoStartTask();
	}

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsRunAtStartup"));
}

bool SettingsViewModel::IsPortableMode() const noexcept {
	return (int32_t)AppSettings::Get().IsPortableMode();
}

void SettingsViewModel::IsPortableMode(bool value) noexcept {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsPortableMode() == value) {
		return;
	}

	settings.IsPortableMode(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsPortableMode"));
}

bool SettingsViewModel::IsShowTrayIcon() const noexcept {
	return AppSettings::Get().IsShowTrayIcon();
}

void SettingsViewModel::IsShowTrayIcon(bool value) noexcept {
	AppSettings::Get().IsShowTrayIcon(value);
}

bool SettingsViewModel::IsSimulateExclusiveFullscreen() const noexcept {
	return AppSettings::Get().IsSimulateExclusiveFullscreen();
}

void SettingsViewModel::IsSimulateExclusiveFullscreen(bool value) noexcept {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsSimulateExclusiveFullscreen() == value) {
		return;
	}

	settings.IsSimulateExclusiveFullscreen(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsSimulateExclusiveFullscreen"));
}

bool SettingsViewModel::IsBreakpointMode() const noexcept {
	return AppSettings::Get().IsBreakpointMode();
}

void SettingsViewModel::IsBreakpointMode(bool value) noexcept {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsBreakpointMode() == value) {
		return;
	}

	settings.IsBreakpointMode(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsBreakpointMode"));
}

bool SettingsViewModel::IsDisableEffectCache() const noexcept {
	return AppSettings::Get().IsDisableEffectCache();
}

void SettingsViewModel::IsDisableEffectCache(bool value) noexcept {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsDisableEffectCache() == value) {
		return;
	}

	settings.IsDisableEffectCache(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDisableEffectCache"));
}

bool SettingsViewModel::IsSaveEffectSources() const noexcept {
	return AppSettings::Get().IsSaveEffectSources();
}

void SettingsViewModel::IsSaveEffectSources(bool value) noexcept {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsSaveEffectSources() == value) {
		return;
	}

	settings.IsSaveEffectSources(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsSaveEffectSources"));
}

bool SettingsViewModel::IsWarningsAreErrors() const noexcept {
	return AppSettings::Get().IsWarningsAreErrors();
}

void SettingsViewModel::IsWarningsAreErrors(bool value) noexcept {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsWarningsAreErrors() == value) {
		return;
	}

	settings.IsWarningsAreErrors(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsWarningsAreErrors"));
}

}
