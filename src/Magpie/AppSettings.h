#pragma once
#include <winrt/Magpie.h>
#include "Event.h"
#include "Shortcut.h"
#include "Profile.h"
#include <parallel_hashmap/phmap.h>
#include <rapidjson/document.h>

namespace Magpie {

struct ScalingMode;

enum class AppTheme {
	Light,
	Dark,
	System
};

struct _AppSettingsData {
	_AppSettingsData();
	virtual ~_AppSettingsData();

	std::array<Shortcut, (size_t)winrt::Magpie::ShortcutAction::COUNT_OR_NONE> _shortcuts;

	std::vector<ScalingMode> _scalingModes;

	Profile _defaultProfile;
	std::vector<Profile> _profiles;

	std::wstring _configDir;
	std::wstring _configPath;

	// LocalizationService::SupportedLanguages 索引
	// -1 表示使用系统设置
	int _language = -1;

	// 保存窗口中心点和 DPI 无关的窗口尺寸
	winrt::Point _mainWindowCenter{};
	// 小于零表示默认位置和尺寸
	winrt::Size _mainWindowSizeInDips{ -1.0f,-1.0f };
	
	AppTheme _theme = AppTheme::System;
	// 必须在 1~5 之间
	uint32_t _countdownSeconds = 3;

	// 上一次自动检查更新的日期
	std::chrono::system_clock::time_point _updateCheckDate;

	DuplicateFrameDetectionMode _duplicateFrameDetectionMode =
		DuplicateFrameDetectionMode::Dynamic;

	float _minFrameRate = 10.0f;
	
	bool _isPortableMode = false;
	bool _isAlwaysRunAsAdmin = false;
	bool _isDeveloperMode = false;
	bool _isDebugMode = false;
	bool _isBenchmarkMode = false;
	bool _isEffectCacheDisabled = false;
	bool _isFontCacheDisabled = false;
	bool _isSaveEffectSources = false;
	bool _isWarningsAreErrors = false;
	bool _isAllowScalingMaximized = false;
	bool _isSimulateExclusiveFullscreen = false;
	bool _isInlineParams = false;
	bool _isShowNotifyIcon = true;
	bool _isMainWindowMaximized = false;
	bool _isAutoCheckForUpdates = true;
	bool _isCheckForPreviewUpdates = false;
	bool _isStatisticsForDynamicDetectionEnabled = false;
	bool _isFP16Disabled = false;
};

class AppSettings : private _AppSettingsData {
public:
	static AppSettings& Get() noexcept {
		static AppSettings instance;
		return instance;
	}

	virtual ~AppSettings();

	bool Initialize() noexcept;

	bool Save() noexcept;

	winrt::fire_and_forget SaveAsync() noexcept;

	const std::wstring& ConfigDir() const noexcept {
		return _configDir;
	}

	bool IsPortableMode() const noexcept {
		return _isPortableMode;
	}

	void IsPortableMode(bool value) noexcept;

	int Language() const noexcept {
		return _language;
	}

	void Language(int);

	AppTheme Theme() const noexcept {
		return _theme;
	}
	void Theme(AppTheme value);

	winrt::Point MainWindowCenter() const noexcept {
		return _mainWindowCenter;
	}

	winrt::Size MainWindowSizeInDips() const noexcept {
		return _mainWindowSizeInDips;
	}

	bool IsMainWindowMaximized() const noexcept {
		return _isMainWindowMaximized;
	}

	const Shortcut& GetShortcut(winrt::Magpie::ShortcutAction action) const {
		return _shortcuts[(size_t)action];
	}

	void SetShortcut(winrt::Magpie::ShortcutAction action, const Shortcut& value);

	uint32_t CountdownSeconds() const noexcept {
		return _countdownSeconds;
	}

	void CountdownSeconds(uint32_t value) noexcept;

	bool IsDeveloperMode() const noexcept {
		return _isDeveloperMode;
	}

	void IsDeveloperMode(bool value) noexcept;

	bool IsDebugMode() const noexcept {
		return _isDebugMode;
	}

	void IsDebugMode(bool value) noexcept {
		_isDebugMode = value;
		SaveAsync();
	}

	bool IsBenchmarkMode() const noexcept {
		return _isBenchmarkMode;
	}

	void IsBenchmarkMode(bool value) noexcept {
		_isBenchmarkMode = value;
		SaveAsync();
	}

	bool IsEffectCacheDisabled() const noexcept {
		return _isEffectCacheDisabled;
	}

	void IsEffectCacheDisabled(bool value) noexcept {
		_isEffectCacheDisabled = value;
		SaveAsync();
	}

	bool IsFontCacheDisabled() const noexcept {
		return _isFontCacheDisabled;
	}

	void IsFontCacheDisabled(bool value) noexcept {
		_isFontCacheDisabled = value;
		SaveAsync();
	}

	bool IsSaveEffectSources() const noexcept {
		return _isSaveEffectSources;
	}

	void IsSaveEffectSources(bool value) noexcept {
		_isSaveEffectSources = value;
		SaveAsync();
	}

	bool IsWarningsAreErrors() const noexcept {
		return _isWarningsAreErrors;
	}

	void IsWarningsAreErrors(bool value) noexcept {
		_isWarningsAreErrors = value;
		SaveAsync();
	}

	bool IsFP16Disabled() const noexcept {
		return _isFP16Disabled;
	}

	void IsFP16Disabled(bool value) noexcept {
		_isFP16Disabled = value;
		SaveAsync();
	}

	bool IsAllowScalingMaximized() const noexcept {
		return _isAllowScalingMaximized;
	}

	void IsAllowScalingMaximized(bool value) noexcept {
		_isAllowScalingMaximized = value;
		SaveAsync();
	}

	bool IsSimulateExclusiveFullscreen() const noexcept {
		return _isSimulateExclusiveFullscreen;
	}

	void IsSimulateExclusiveFullscreen(bool value) noexcept {
		_isSimulateExclusiveFullscreen = value;
		SaveAsync();
	}

	Profile& DefaultProfile() noexcept {
		return _defaultProfile;
	}

	std::vector<Profile>& Profiles() noexcept {
		return _profiles;
	}

	bool IsAlwaysRunAsAdmin() const noexcept {
		return _isAlwaysRunAsAdmin;
	}

	void IsAlwaysRunAsAdmin(bool value) noexcept;

	bool IsShowNotifyIcon() const noexcept {
		return _isShowNotifyIcon;
	}

	void IsShowNotifyIcon(bool value) noexcept;

	bool IsInlineParams() const noexcept {
		return _isInlineParams;
	}

	void IsInlineParams(bool value) noexcept {
		_isInlineParams = value;
		SaveAsync();
	}

	std::vector<ScalingMode>& ScalingModes() noexcept {
		return _scalingModes;
	}

	bool IsAutoCheckForUpdates() const noexcept {
		return _isAutoCheckForUpdates;
	}

	void IsAutoCheckForUpdates(bool value) noexcept {
		_isAutoCheckForUpdates = value;
		IsAutoCheckForUpdatesChanged.Invoke(value);
		SaveAsync();
	}

	bool IsCheckForPreviewUpdates() const noexcept {
		return _isCheckForPreviewUpdates;
	}

	void IsCheckForPreviewUpdates(bool value) noexcept {
		_isCheckForPreviewUpdates = value;
		SaveAsync();
	}

	std::chrono::system_clock::time_point UpdateCheckDate() const noexcept {
		return _updateCheckDate;
	}

	void UpdateCheckDate(std::chrono::system_clock::time_point value) noexcept {
		_updateCheckDate = value;
	}

	DuplicateFrameDetectionMode DuplicateFrameDetectionMode() const noexcept {
		return _duplicateFrameDetectionMode;
	}

	void DuplicateFrameDetectionMode(enum DuplicateFrameDetectionMode value) noexcept {
		_duplicateFrameDetectionMode = value;
		SaveAsync();
	}

	bool IsStatisticsForDynamicDetectionEnabled() const noexcept {
		return _isStatisticsForDynamicDetectionEnabled;
	}

	void IsStatisticsForDynamicDetectionEnabled(bool value) noexcept {
		_isStatisticsForDynamicDetectionEnabled = value;
		SaveAsync();
	}

	float MinFrameRate() const noexcept {
		return _minFrameRate;
	}

	void MinFrameRate(float value) noexcept {
		_minFrameRate = value;
		SaveAsync();
	}

	Event<AppTheme> ThemeChanged;
	Event<winrt::Magpie::ShortcutAction> ShortcutChanged;
	Event<uint32_t> CountdownSecondsChanged;
	Event<bool> IsShowNotifyIconChanged;
	Event<bool> IsAutoCheckForUpdatesChanged;

private:
	AppSettings() = default;

	AppSettings(const AppSettings&) = delete;
	AppSettings(AppSettings&&) = delete;

	void _UpdateWindowPlacement() noexcept;
	bool _Save(const _AppSettingsData& data) noexcept;

	void _LoadSettings(const rapidjson::GenericObject<true, rapidjson::Value>& root) noexcept;
	bool _LoadProfile(
		const rapidjson::GenericObject<true, rapidjson::Value>& profileObj,
		Profile& profile,
		bool isDefault = false
	) const noexcept;
	bool _SetDefaultShortcuts() noexcept;
	void _SetDefaultScalingModes() noexcept;

	bool _UpdateConfigPath(std::wstring* existingConfigPath = nullptr) noexcept;

	// 用于同步保存
	wil::srwlock _saveLock;
};

}
