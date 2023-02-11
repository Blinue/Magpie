#include "pch.h"
#include "SettingsViewModel.h"
#if __has_include("SettingsViewModel.g.cpp")
#include "SettingsViewModel.g.cpp"
#endif
#include "AppSettings.h"
#include "AutoStartHelper.h"
#include "Win32Utils.h"
#include "CommonSharedConstants.h"
#include "LocalizationService.h"

using namespace winrt;
using namespace Windows::ApplicationModel::Resources;

namespace winrt::Magpie::App::implementation {

SettingsViewModel::SettingsViewModel() {
	_UpdateStartupOptions();
}

IVector<IInspectable> SettingsViewModel::Languages() const {
	std::span<const wchar_t*> tags = LocalizationService::Get().GetSupportedLanguages();

	std::vector<IInspectable> languages;
	languages.reserve(tags.size() + 1);

	ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView();
	languages.push_back(box_value(resourceLoader.GetString(L"Settings_General_Language_Default")));
	for (const wchar_t* tag : tags) {
		Windows::Globalization::Language language(tag);
		languages.push_back(box_value(language.NativeName()));
	}
	return single_threaded_vector(std::move(languages));;
}

int SettingsViewModel::Language() const noexcept {
	return AppSettings::Get().Language() + 1;
}

void SettingsViewModel::Language(int value) {
	if (value < 0) {
		return;
	}

	AppSettings::Get().Language(value - 1);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Language"));
}

int SettingsViewModel::Theme() const noexcept {
	return (int)AppSettings::Get().Theme();
}

void SettingsViewModel::Theme(int value) {
	if (value < 0) {
		return;
	}

	AppSettings::Get().Theme((uint32_t)value);
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
	return AppSettings::Get().IsPortableMode();
}

void SettingsViewModel::IsPortableMode(bool value) noexcept {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsPortableMode() == value) {
		return;
	}

	settings.IsPortableMode(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsPortableMode"));
}

fire_and_forget SettingsViewModel::OpenConfigLocation() const noexcept {
	std::wstring configPath = AppSettings::Get().ConfigDir() + CommonSharedConstants::CONFIG_NAME;
	co_await resume_background();
	Win32Utils::OpenFolderAndSelectFile(configPath.c_str());
}

bool SettingsViewModel::IsShowTrayIcon() const noexcept {
	return AppSettings::Get().IsShowTrayIcon();
}

void SettingsViewModel::IsShowTrayIcon(bool value) noexcept {
	AppSettings::Get().IsShowTrayIcon(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsShowTrayIcon"));

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

bool SettingsViewModel::IsDebugMode() const noexcept {
	return AppSettings::Get().IsDebugMode();
}

void SettingsViewModel::IsDebugMode(bool value) noexcept {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsDebugMode() == value) {
		return;
	}

	settings.IsDebugMode(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDebugMode"));
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
