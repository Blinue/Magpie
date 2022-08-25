#include "pch.h"
#include "SettingsViewModel.h"
#if __has_include("SettingsViewModel.g.cpp")
#include "SettingsViewModel.g.cpp"
#endif
#include "AppSettings.h"
#include "AutoStartHelper.h"
#include "Win32Utils.h"
#include "CommonSharedConstants.h"


namespace winrt::Magpie::App::implementation {

SettingsViewModel::SettingsViewModel() {
	_UpdateStartupOptions();
}

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

void SettingsViewModel::IsRunAtStartup(bool value) noexcept {
	if (value) {
		AutoStartHelper::EnableAutoStart(
			AppSettings::Get().IsAlwaysRunAsElevated(),
			_isMinimizeAtStartup ? CommonSharedConstants::OPTION_MINIMIZE_TO_TRAY_AT_STARTUP : nullptr
		);
	} else {
		AutoStartHelper::DisableAutoStart();
	}

	_UpdateStartupOptions();

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsMinimizeAtStartupEnabled"));
}

void SettingsViewModel::IsMinimizeAtStartup(bool value) noexcept {
	if (!_isRunAtStartup) {
		return;
	}

	AutoStartHelper::EnableAutoStart(
		AppSettings::Get().IsAlwaysRunAsElevated(),
		value ? CommonSharedConstants::OPTION_MINIMIZE_TO_TRAY_AT_STARTUP : nullptr
	);

	_UpdateStartupOptions();
}

bool SettingsViewModel::IsMinimizeAtStartupEnabled() const noexcept {
	return IsRunAtStartup() && IsShowTrayIcon();
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

void SettingsViewModel::OpenConfigLocation() const noexcept {
	ShellExecute(NULL, L"explore", AppSettings::Get().ConfigDir().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

bool SettingsViewModel::IsShowTrayIcon() const noexcept {
	return AppSettings::Get().IsShowTrayIcon();
}

void SettingsViewModel::IsShowTrayIcon(bool value) noexcept {
	AppSettings::Get().IsShowTrayIcon(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsMinimizeAtStartupEnabled"));

	if (_isRunAtStartup) {
		AutoStartHelper::EnableAutoStart(AppSettings::Get().IsAlwaysRunAsElevated(), nullptr);
		_UpdateStartupOptions();
	}

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsMinimizeAtStartupEnabled"));
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

bool SettingsViewModel::IsInlineParams() const noexcept {
	return AppSettings::Get().IsInlineParams();
}

void SettingsViewModel::IsInlineParams(bool value) noexcept {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsInlineParams() == value) {
		return;
	}

	settings.IsInlineParams(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsInlineParams"));
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

void SettingsViewModel::_UpdateStartupOptions() noexcept {
	std::wstring arguments;
	_isRunAtStartup = AutoStartHelper::IsAutoStartEnabled(arguments);
	if (_isRunAtStartup) {
		_isMinimizeAtStartup = arguments == CommonSharedConstants::OPTION_MINIMIZE_TO_TRAY_AT_STARTUP;
	} else {
		_isMinimizeAtStartup = false;
	}

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsRunAtStartup"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsMinimizeAtStartup"));
}

}
