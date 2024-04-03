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
#include "ScalingService.h"

namespace winrt::Magpie::App::implementation {

SettingsViewModel::SettingsViewModel() {
	_UpdateStartupOptions();
}

IVector<IInspectable> SettingsViewModel::Languages() const {
	std::span<const wchar_t*> tags = LocalizationService::Get().SupportedLanguages();

	std::vector<IInspectable> languages;
	languages.reserve(tags.size() + 1);

	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	languages.push_back(box_value(resourceLoader.GetString(L"Settings_General_Language_System")));
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
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"RequireRestart"));
}

bool SettingsViewModel::RequireRestart() const noexcept {
	static int initLanguage = AppSettings::Get().Language();
	return AppSettings::Get().Language() != initLanguage;
}

void SettingsViewModel::Restart() const {
	Application::Current().as<App>().Restart();
}

int SettingsViewModel::Theme() const noexcept {
	switch (AppSettings::Get().Theme()) {
	case Theme::System:
		return 0;
	case Theme::Light:
		return 1;
	case Theme::Dark:
		return 2;
	default:
		return 0;
	}
}

void SettingsViewModel::Theme(int value) {
	if (value < 0) {
		return;
	}

	Magpie::App::Theme theme;
	switch (value) {
	case 1:
		theme = Theme::Light;
		break;
	case 2:
		theme = Theme::Dark;
		break;
	default:
		theme = Theme::System;
		break;
	}

	AppSettings::Get().Theme(theme);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Theme"));
}

void SettingsViewModel::IsRunAtStartup(bool value) {
	if (value) {
		AutoStartHelper::EnableAutoStart(
			AppSettings::Get().IsAlwaysRunAsAdmin(),
			_isMinimizeAtStartup ? CommonSharedConstants::OPTION_LAUNCH_WITHOUT_WINDOW : nullptr
		);
	} else {
		AutoStartHelper::DisableAutoStart();
	}

	_UpdateStartupOptions();

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsMinimizeAtStartupEnabled"));
}

void SettingsViewModel::IsMinimizeAtStartup(bool value) {
	if (!_isRunAtStartup) {
		return;
	}

	AutoStartHelper::EnableAutoStart(
		AppSettings::Get().IsAlwaysRunAsAdmin(),
		value ? CommonSharedConstants::OPTION_LAUNCH_WITHOUT_WINDOW : nullptr
	);

	_UpdateStartupOptions();
}

bool SettingsViewModel::IsMinimizeAtStartupEnabled() const noexcept {
	return IsRunAtStartup() && IsShowNotifyIcon();
}

bool SettingsViewModel::IsPortableMode() const noexcept {
	return AppSettings::Get().IsPortableMode();
}

void SettingsViewModel::IsPortableMode(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsPortableMode() == value) {
		return;
	}

	settings.IsPortableMode(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsPortableMode"));
}

fire_and_forget SettingsViewModel::OpenConfigLocation() const noexcept {
	std::wstring configPath = AppSettings::Get().ConfigDir() + CommonSharedConstants::CONFIG_FILENAME;
	co_await resume_background();
	Win32Utils::OpenFolderAndSelectFile(configPath.c_str());
}

bool SettingsViewModel::IsShowNotifyIcon() const noexcept {
	return AppSettings::Get().IsShowNotifyIcon();
}

void SettingsViewModel::IsShowNotifyIcon(bool value) {
	AppSettings::Get().IsShowNotifyIcon(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsShowNotifyIcon"));

	if (_isRunAtStartup) {
		AutoStartHelper::EnableAutoStart(AppSettings::Get().IsAlwaysRunAsAdmin(), nullptr);
		_UpdateStartupOptions();
	}

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsMinimizeAtStartupEnabled"));
}

bool SettingsViewModel::IsProcessElevated() const noexcept {
	return Win32Utils::IsProcessElevated();
}

bool SettingsViewModel::IsAlwaysRunAsAdmin() const noexcept {
	return AppSettings::Get().IsAlwaysRunAsAdmin();
}

void SettingsViewModel::IsAlwaysRunAsAdmin(bool value) {
	AppSettings::Get().IsAlwaysRunAsAdmin(value);
}

bool SettingsViewModel::IsAllowScalingMaximized() const noexcept {
	return AppSettings::Get().IsAllowScalingMaximized();
}

void SettingsViewModel::IsAllowScalingMaximized(bool value) {
	AppSettings::Get().IsAllowScalingMaximized(value);

	if (value) {
		ScalingService::Get().CheckForeground();
	}
}

bool SettingsViewModel::IsSimulateExclusiveFullscreen() const noexcept {
	return AppSettings::Get().IsSimulateExclusiveFullscreen();
}

void SettingsViewModel::IsSimulateExclusiveFullscreen(bool value) {
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

void SettingsViewModel::IsInlineParams(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsInlineParams() == value) {
		return;
	}

	settings.IsInlineParams(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsInlineParams"));
}

bool SettingsViewModel::IsDeveloperMode() const noexcept {
	return AppSettings::Get().IsDeveloperMode();
}

void SettingsViewModel::IsDeveloperMode(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsDeveloperMode() == value) {
		return;
	}

	settings.IsDeveloperMode(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDeveloperMode"));
}

bool SettingsViewModel::IsDebugMode() const noexcept {
	return AppSettings::Get().IsDebugMode();
}

void SettingsViewModel::IsDebugMode(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsDebugMode() == value) {
		return;
	}

	settings.IsDebugMode(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDebugMode"));
}

bool SettingsViewModel::IsEffectCacheDisabled() const noexcept {
	return AppSettings::Get().IsEffectCacheDisabled();
}

void SettingsViewModel::IsEffectCacheDisabled(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsEffectCacheDisabled() == value) {
		return;
	}

	settings.IsEffectCacheDisabled(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsEffectCacheDisabled"));
}

bool SettingsViewModel::IsFontCacheDisabled() const noexcept {
	return AppSettings::Get().IsFontCacheDisabled();
}

void SettingsViewModel::IsFontCacheDisabled(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsFontCacheDisabled() == value) {
		return;
	}

	settings.IsFontCacheDisabled(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsFontCacheDisabled"));
}

bool SettingsViewModel::IsSaveEffectSources() const noexcept {
	return AppSettings::Get().IsSaveEffectSources();
}

void SettingsViewModel::IsSaveEffectSources(bool value) {
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

void SettingsViewModel::IsWarningsAreErrors(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsWarningsAreErrors() == value) {
		return;
	}

	settings.IsWarningsAreErrors(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsWarningsAreErrors"));
}

int SettingsViewModel::DuplicateFrameDetectionMode() const noexcept {
	return (int)AppSettings::Get().DuplicateFrameDetectionMode();
}

void SettingsViewModel::DuplicateFrameDetectionMode(int value) {
	if (value < 0) {
		return;
	}

	const auto mode = (::Magpie::Core::DuplicateFrameDetectionMode)value;

	AppSettings& settings = AppSettings::Get();
	if (settings.DuplicateFrameDetectionMode() == mode) {
		return;
	}

	settings.DuplicateFrameDetectionMode(mode);
	
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"DuplicateFrameDetectionMode"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDynamicDection"));

	if (mode != ::Magpie::Core::DuplicateFrameDetectionMode::Dynamic) {
		settings.IsStatisticsForDynamicDetectionEnabled(false);
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsStatisticsForDynamicDetectionEnabled"));
	}
}

bool SettingsViewModel::IsDynamicDection() const noexcept {
	return AppSettings::Get().DuplicateFrameDetectionMode() == ::Magpie::Core::DuplicateFrameDetectionMode::Dynamic;
}

bool SettingsViewModel::IsStatisticsForDynamicDetectionEnabled() const noexcept {
	return AppSettings::Get().IsStatisticsForDynamicDetectionEnabled();
}

void SettingsViewModel::IsStatisticsForDynamicDetectionEnabled(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsStatisticsForDynamicDetectionEnabled() == value) {
		return;
	}

	settings.IsStatisticsForDynamicDetectionEnabled(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsStatisticsForDynamicDetectionEnabled"));
}

void SettingsViewModel::_UpdateStartupOptions() {
	std::wstring arguments;
	_isRunAtStartup = AutoStartHelper::IsAutoStartEnabled(arguments);
	if (_isRunAtStartup) {
		_isMinimizeAtStartup = arguments == CommonSharedConstants::OPTION_LAUNCH_WITHOUT_WINDOW;
	} else {
		_isMinimizeAtStartup = false;
	}

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsRunAtStartup"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsMinimizeAtStartup"));
}

}
