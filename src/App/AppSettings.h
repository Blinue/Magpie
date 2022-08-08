#pragma once
#include <winrt/Magpie.App.h>
#include "WinRTUtils.h"
#include "HotkeySettings.h"
#include "ScalingProfile.h"


namespace winrt::Magpie::App {

class AppSettings {
public:
	static AppSettings& Get() {
		static AppSettings instance;
		return instance;
	}

	bool Initialize();

	bool Save();

	hstring WorkingDir() const noexcept {
		return _workingDir;
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

	Rect WindowRect() const noexcept {
		return _windowRect;
	}

	void WindowRect(const Rect& value) noexcept {
		_windowRect = value;
	}

	bool IsWindowMaximized() const noexcept {
		return _isWindowMaximized;
	}

	void IsWindowMaximized(bool value) noexcept {
		_isWindowMaximized = value;
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

	bool IsBreakpointMode() const noexcept {
		return _isBreakpointMode;
	}

	void IsBreakpointMode(bool value) noexcept {
		_isBreakpointMode = value;
	}

	bool IsDisableEffectCache() const noexcept {
		return _isDisableEffectCache;
	}

	void IsDisableEffectCache(bool value) noexcept {
		_isDisableEffectCache = value;
	}

	bool IsSaveEffectSources() const noexcept {
		return _isSaveEffectSources;
	}

	void IsSaveEffectSources(bool value) noexcept {
		_isSaveEffectSources = value;
	}

	bool IsWarningsAreErrors() const noexcept {
		return _isWarningsAreErrors;
	}

	void IsWarningsAreErrors(bool value) noexcept {
		_isWarningsAreErrors = value;
	}

	bool IsSimulateExclusiveFullscreen() const noexcept {
		return _isSimulateExclusiveFullscreen;
	}

	void IsSimulateExclusiveFullscreen(bool value) noexcept {
		_isSimulateExclusiveFullscreen = value;
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

private:
	AppSettings() = default;

	AppSettings(const AppSettings&) = delete;
	AppSettings(AppSettings&&) = delete;

	bool _LoadSettings(std::string text);
	void _SetDefaultHotkeys();

	bool _isPortableMode = false;
	hstring _workingDir;

	// 0: 浅色
	// 1: 深色
	// 2: 系统
	uint32_t _theme = 2;
	event<delegate<uint32_t>> _themeChangedEvent;

	Rect _windowRect{ CW_USEDEFAULT,CW_USEDEFAULT,1280,820 };
	bool _isWindowMaximized = false;

	std::array<HotkeySettings, (size_t)HotkeyAction::COUNT_OR_NONE> _hotkeys;
	event<delegate<HotkeyAction>> _hotkeyChangedEvent;

	bool _isAutoRestore = false;
	event<delegate<bool>> _isAutoRestoreChangedEvent;

	uint32_t _downCount = 5;
	event<delegate<uint32_t>> _downCountChangedEvent;

	bool _isShowTrayIcon = true;
	event<delegate<bool>> _isShowTrayIconChangedEvent;
	bool _isAlwaysRunAsElevated = false;
	bool _isBreakpointMode = false;
	bool _isDisableEffectCache = false;
	bool _isSaveEffectSources = false;
	bool _isWarningsAreErrors = false;

	bool _isSimulateExclusiveFullscreen = false;

	ScalingProfile _defaultScalingProfile;
	std::vector<ScalingProfile> _scalingProfiles;
};

}
