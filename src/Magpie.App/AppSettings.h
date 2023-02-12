#pragma once
#include <winrt/Magpie.App.h>
#include "WinRTUtils.h"
#include "Shortcut.h"
#include "Profile.h"
#include <parallel_hashmap/phmap.h>
#include <rapidjson/document.h>
#include "Win32Utils.h"

namespace winrt::Magpie::App {

struct ScalingMode;

struct _AppSettingsData {
	_AppSettingsData();
	virtual ~_AppSettingsData();

	std::array<Shortcut, (size_t)ShortcutAction::COUNT_OR_NONE> _shortcuts;

	::Magpie::Core::DownscalingEffect _downscalingEffect;
	std::vector<ScalingMode> _scalingModes;

	Profile _defaultProfile;
	std::vector<Profile> _profiles;

	std::wstring _configDir;
	std::wstring _configPath;

	// LocalizationService::GetSupportedLanguages 索引
	// -1 表示使用系统设置
	int _language = -1;

	// X, Y, 长, 高
	RECT _windowRect{ CW_USEDEFAULT,CW_USEDEFAULT,1280,820 };

	// 0: 浅色
	// 1: 深色
	// 2: 系统
	uint32_t _theme = 2;
	// 必须在 1~5 之间
	uint32_t _countdownSeconds = 3;

	// 上一次自动检查更新的日期
	std::chrono::system_clock::time_point _updateCheckDate;
	
	bool _isPortableMode = false;
	bool _isAlwaysRunAsElevated = false;
	bool _isDebugMode = false;
	bool _isDisableEffectCache = false;
	bool _isSaveEffectSources = false;
	bool _isWarningsAreErrors = false;
	bool _isSimulateExclusiveFullscreen = false;
	bool _isInlineParams = false;
	bool _isShowTrayIcon = true;
	bool _isAutoRestore = false;
	bool _isWindowMaximized = false;
	bool _isAutoCheckForUpdates = false;
	bool _isCheckForPreviewUpdates = false;
};

class AppSettings : private _AppSettingsData {
public:
	static AppSettings& Get() noexcept {
		static AppSettings instance;
		return instance;
	}

	virtual ~AppSettings();

	bool Initialize();

	bool Save();

	fire_and_forget SaveAsync();

	const std::wstring& ConfigDir() const noexcept {
		return _configDir;
	}

	bool IsPortableMode() const noexcept {
		return _isPortableMode;
	}

	void IsPortableMode(bool value);

	int Language() const noexcept {
		return _language;
	}

	void Language(int);

	event_token LanguageChanged(delegate<int> const& handler) {
		return _languageChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker LanguageChanged(auto_revoke_t, delegate<int> const& handler) {
		event_token token = LanguageChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			LanguageChanged(token);
		});
	}

	void LanguageChanged(event_token const& token) {
		_languageChangedEvent.remove(token);
	}

	uint32_t Theme() const noexcept {
		return _theme;
	}
	void Theme(uint32_t value);

	event_token ThemeChanged(delegate<uint32_t> const& handler) {
		return _themeChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker ThemeChanged(auto_revoke_t, delegate<uint32_t> const& handler) {
		event_token token = ThemeChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			ThemeChanged(token);
		});
	}

	void ThemeChanged(event_token const& token) {
		_themeChangedEvent.remove(token);
	}

	const RECT& WindowRect() const noexcept {
		return _windowRect;
	}

	bool IsWindowMaximized() const noexcept {
		return _isWindowMaximized;
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

	bool IsDebugMode() const noexcept {
		return _isDebugMode;
	}

	void IsDebugMode(bool value) noexcept {
		_isDebugMode = value;
		SaveAsync();
	}

	bool IsDisableEffectCache() const noexcept {
		return _isDisableEffectCache;
	}

	void IsDisableEffectCache(bool value) noexcept {
		_isDisableEffectCache = value;
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

	bool IsAlwaysRunAsElevated() const noexcept {
		return _isAlwaysRunAsElevated;
	}

	void IsAlwaysRunAsElevated(bool value) noexcept;

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

	::Magpie::Core::DownscalingEffect& DownscalingEffect() noexcept {
		return _downscalingEffect;
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

private:
	AppSettings() = default;

	AppSettings(const AppSettings&) = delete;
	AppSettings(AppSettings&&) = delete;

	void _UpdateWindowPlacement() noexcept;
	bool _Save(const _AppSettingsData& data) noexcept;

	void _LoadSettings(const rapidjson::GenericObject<true, rapidjson::Value>& root, uint32_t version);
	bool _LoadProfile(
		const rapidjson::GenericObject<true, rapidjson::Value>& profileObj,
		Profile& profile,
		bool isDefault = false
	);
	bool _SetDefaultShortcuts();
	void _SetDefaultScalingModes();

	void _UpdateConfigPath() noexcept;

	// 用于同步保存
	Win32Utils::SRWMutex _saveMutex;

	event<delegate<int>> _languageChangedEvent;
	event<delegate<uint32_t>> _themeChangedEvent;
	event<delegate<ShortcutAction>> _shortcutChangedEvent;
	event<delegate<bool>> _isAutoRestoreChangedEvent;
	event<delegate<uint32_t>> _countdownSecondsChangedEvent;
	event<delegate<bool>> _isShowTrayIconChangedEvent;
	event<delegate<bool>> _isAutoCheckForUpdatesChangedEvent;
};

}
