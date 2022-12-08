#pragma once
#include <winrt/Magpie.UI.h>
#include "WinRTUtils.h"
#include "HotkeySettings.h"
#include "ScalingProfile.h"
#include <parallel_hashmap/phmap.h>
#include <rapidjson/document.h>
#include "Win32Utils.h"

namespace winrt::Magpie::UI {

struct ScalingMode;

struct _AppSettingsData {
	_AppSettingsData();
	virtual ~_AppSettingsData();

	std::array<HotkeySettings, (size_t)HotkeyAction::COUNT_OR_NONE> _hotkeys;

	::Magpie::Core::DownscalingEffect _downscalingEffect;
	std::vector<ScalingMode> _scalingModes;

	ScalingProfile _defaultScalingProfile;
	std::vector<ScalingProfile> _scalingProfiles;

	std::wstring _configDir;
	std::wstring _configPath;

	// X, Y, 长, 高
	RECT _windowRect{ CW_USEDEFAULT,CW_USEDEFAULT,1280,820 };

	// 0: 浅色
	// 1: 深色
	// 2: 系统
	uint32_t _theme = 2;
	// 必须在 1~5 之间
	uint32_t _downCount = 3;

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
	bool _isAutoDownloadUpdates = false;
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

	const HotkeySettings& GetHotkey(HotkeyAction action) const {
		return _hotkeys[(size_t)action];
	}

	void SetHotkey(HotkeyAction action, const HotkeySettings& value);

	event_token HotkeyChanged(delegate<HotkeyAction> const& handler) {
		return _hotkeyChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker HotkeyChanged(auto_revoke_t, delegate<HotkeyAction> const& handler) {
		event_token token = HotkeyChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			HotkeyChanged(token);
		});
	}

	void HotkeyChanged(event_token const& token) {
		_hotkeyChangedEvent.remove(token);
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

	uint32_t DownCount() const noexcept {
		return _downCount;
	}

	void DownCount(uint32_t value) noexcept;

	event_token DownCountChanged(delegate<uint32_t> const& handler) {
		return _downCountChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker DownCountChanged(auto_revoke_t, delegate<uint32_t> const& handler) {
		event_token token = DownCountChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			DownCountChanged(token);
		});
	}

	void DownCountChanged(event_token const& token) {
		_downCountChangedEvent.remove(token);
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

	ScalingProfile& DefaultScalingProfile() noexcept {
		return _defaultScalingProfile;
	}

	std::vector<ScalingProfile>& ScalingProfiles() noexcept {
		return _scalingProfiles;
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

	bool IsAutoDownloadUpdates() const noexcept {
		return _isAutoDownloadUpdates;
	}

	void IsAutoDownloadUpdates(bool value) noexcept {
		_isAutoDownloadUpdates = value;
		SaveAsync();
	}

	bool IsCheckForPreviewUpdates() const noexcept {
		return _isCheckForPreviewUpdates;
	}

	void IsCheckForPreviewUpdates(bool value) noexcept {
		_isCheckForPreviewUpdates = value;
		SaveAsync();
	}
private:
	AppSettings() = default;

	AppSettings(const AppSettings&) = delete;
	AppSettings(AppSettings&&) = delete;

	void _UpdateWindowPlacement() noexcept;
	bool _Save(const _AppSettingsData& data) noexcept;

	void _LoadSettings(const rapidjson::GenericObject<true, rapidjson::Value>& root, uint32_t version);
	bool _LoadScalingProfile(
		const rapidjson::GenericObject<true, rapidjson::Value>& scalingProfileObj,
		ScalingProfile& scalingProfile,
		bool isDefault = false
	);
	bool _SetDefaultHotkeys();
	void _SetDefaultScalingModes();

	void _UpdateConfigPath() noexcept;

	// 用于同步保存
	Win32Utils::SRWMutex _saveMutex;

	event<delegate<uint32_t>> _themeChangedEvent;
	event<delegate<HotkeyAction>> _hotkeyChangedEvent;
	event<delegate<bool>> _isAutoRestoreChangedEvent;
	event<delegate<uint32_t>> _downCountChangedEvent;
	event<delegate<bool>> _isShowTrayIconChangedEvent;
};

}
