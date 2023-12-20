#pragma once
#include <winrt/Magpie.App.h>
#include "WinRTUtils.h"
#include "Shortcut.h"
#include "Profile.h"
#include <parallel_hashmap/phmap.h>
#include <rapidjson/document.h>
#include "Win32Utils.h"
#include <Magpie.Core.h>

namespace winrt::Magpie::App {

struct ScalingMode;

enum class Theme {
	Light,
	Dark,
	System
};

struct _AppSettingsData {
	_AppSettingsData();
	virtual ~_AppSettingsData();

	std::array<Shortcut, (size_t)ShortcutAction::COUNT_OR_NONE> _shortcuts;

	std::vector<ScalingMode> _scalingModes;

	Profile _defaultProfile;
	std::vector<Profile> _profiles;

	std::wstring _configDir;
	std::wstring _configPath;

	// LocalizationService::SupportedLanguages 索引
	// -1 表示使用系统设置
	int _language = -1;

	// 保存窗口中心点和 DPI 无关的窗口尺寸
	Point _mainWindowCenter{};
	// 小于零表示默认位置和尺寸
	Size _mainWindowSizeInDips{ -1.0f,-1.0f };

	Theme _theme = Theme::System;
	// 必须在 1~5 之间
	uint32_t _countdownSeconds = 3;

	// 上一次自动检查更新的日期
	std::chrono::system_clock::time_point _updateCheckDate;

	::Magpie::Core::DuplicateFrameDetectionMode _duplicateFrameDetectionMode =
		::Magpie::Core::DuplicateFrameDetectionMode::Dynamic;
	
	bool _isPortableMode = false;
	bool _isAlwaysRunAsAdmin = false;
	bool _isDeveloperMode = false;
	bool _isDebugMode = false;
	bool _isEffectCacheDisabled = false;
	bool _isFontCacheDisabled = false;
	bool _isSaveEffectSources = false;
	bool _isWarningsAreErrors = false;
	bool _isAllowScalingMaximized = false;
	bool _isSimulateExclusiveFullscreen = false;
	bool _isInlineParams = false;
	bool _isShowTrayIcon = true;
	bool _isAutoRestore = false;
	bool _isMainWindowMaximized = false;
	bool _isAutoCheckForUpdates = true;
	bool _isCheckForPreviewUpdates = false;
	bool _isStatisticsForDynamicDetectionEnabled = false;
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

	fire_and_forget SaveAsync() noexcept;

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

	Theme Theme() const noexcept {
		return _theme;
	}
	void Theme(Magpie::App::Theme value);

	event_token ThemeChanged(delegate<Magpie::App::Theme> const& handler) {
		return _themeChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker ThemeChanged(auto_revoke_t, delegate<Magpie::App::Theme> const& handler) {
		event_token token = ThemeChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			ThemeChanged(token);
		});
	}

	void ThemeChanged(event_token const& token) {
		_themeChangedEvent.remove(token);
	}

	Point MainWindowCenter() const noexcept {
		return _mainWindowCenter;
	}

	Size MainWindowSizeInDips() const noexcept {
		return _mainWindowSizeInDips;
	}

	bool IsWindowMaximized() const noexcept {
		return _isMainWindowMaximized;
	}

	const Shortcut& GetShortcut(ShortcutAction action) const {
		return _shortcuts[(size_t)action];
	}

	void SetShortcut(ShortcutAction action, const Shortcut& value);

	event_token ShortcutChanged(delegate<ShortcutAction> const& handler) {
		return _shortcutChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker ShortcutChanged(auto_revoke_t, delegate<ShortcutAction> const& handler) {
		event_token token = ShortcutChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			ShortcutChanged(token);
		});
	}

	void ShortcutChanged(event_token const& token) {
		_shortcutChangedEvent.remove(token);
	}

	bool IsAutoRestore() const noexcept {
		return _isAutoRestore;
	}

	void IsAutoRestore(bool value) noexcept;

	event_token IsAutoRestoreChanged(delegate<bool> const& handler) {
		return _isAutoRestoreChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker IsAutoRestoreChanged(auto_revoke_t, delegate<bool> const& handler) {
		event_token token = IsAutoRestoreChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			IsAutoRestoreChanged(token);
		});
	}

	void IsAutoRestoreChanged(event_token const& token) {
		_isAutoRestoreChangedEvent.remove(token);
	}

	uint32_t CountdownSeconds() const noexcept {
		return _countdownSeconds;
	}

	void CountdownSeconds(uint32_t value) noexcept;

	event_token CountdownSecondsChanged(delegate<uint32_t> const& handler) {
		return _countdownSecondsChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker CountdownSecondsChanged(auto_revoke_t, delegate<uint32_t> const& handler) {
		event_token token = CountdownSecondsChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			CountdownSecondsChanged(token);
		});
	}

	void CountdownSecondsChanged(event_token const& token) {
		_countdownSecondsChangedEvent.remove(token);
	}

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

	bool IsShowTrayIcon() const noexcept {
		return _isShowTrayIcon;
	}

	void IsShowTrayIcon(bool value) noexcept;

	event_token IsShowTrayIconChanged(delegate<bool> const& handler) {
		return _isShowTrayIconChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker IsShowTrayIconChanged(auto_revoke_t, delegate<bool> const& handler) {
		event_token token = IsShowTrayIconChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			IsShowTrayIconChanged(token);
		});
	}

	void IsShowTrayIconChanged(event_token const& token) {
		_isShowTrayIconChangedEvent.remove(token);
	}

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
		_isAutoCheckForUpdatesChangedEvent(value);
		SaveAsync();
	}

	event_token IsAutoCheckForUpdatesChanged(delegate<bool> const& handler) {
		return _isAutoCheckForUpdatesChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker IsAutoCheckForUpdatesChanged(auto_revoke_t, delegate<bool> const& handler) {
		event_token token = IsAutoCheckForUpdatesChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			IsAutoCheckForUpdatesChanged(token);
		});
	}

	void IsAutoCheckForUpdatesChanged(event_token const& token) {
		_isAutoCheckForUpdatesChangedEvent.remove(token);
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

	::Magpie::Core::DuplicateFrameDetectionMode DuplicateFrameDetectionMode() const noexcept {
		return _duplicateFrameDetectionMode;
	}

	void DuplicateFrameDetectionMode(::Magpie::Core::DuplicateFrameDetectionMode value) noexcept {
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

private:
	AppSettings() = default;

	AppSettings(const AppSettings&) = delete;
	AppSettings(AppSettings&&) = delete;

	void _UpdateWindowPlacement() noexcept;
	bool _Save(const _AppSettingsData& data) noexcept;

	void _LoadSettings(const rapidjson::GenericObject<true, rapidjson::Value>& root, uint32_t version) noexcept;
	bool _LoadProfile(
		const rapidjson::GenericObject<true, rapidjson::Value>& profileObj,
		Profile& profile,
		bool isDefault = false
	) const noexcept;
	bool _SetDefaultShortcuts() noexcept;
	void _SetDefaultScalingModes() noexcept;

	void _UpdateConfigPath() noexcept;

	// 用于同步保存
	Win32Utils::SRWMutex _saveMutex;

	event<delegate<Magpie::App::Theme>> _themeChangedEvent;
	event<delegate<ShortcutAction>> _shortcutChangedEvent;
	event<delegate<bool>> _isAutoRestoreChangedEvent;
	event<delegate<uint32_t>> _countdownSecondsChangedEvent;
	event<delegate<bool>> _isShowTrayIconChangedEvent;
	event<delegate<bool>> _isAutoCheckForUpdatesChangedEvent;
};

}
