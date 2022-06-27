#pragma once
#include "pch.h"
#include "Settings.g.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App::implementation {

struct Settings : SettingsT<Settings> {
	Settings() = default;

	bool Initialize();

	bool Save();

	hstring WorkingDir() const noexcept {
		return _workingDir;
	}

	bool IsPortableMode() const noexcept {
		return _isPortableMode;
	}

	void IsPortableMode(bool value);

	int Theme() const noexcept {
		return _theme;
	}
	void Theme(int value);

	event_token ThemeChanged(EventHandler<int> const& handler) {
		return _themeChangedEvent.add(handler);
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

	Magpie::App::HotkeySettings GetHotkey(HotkeyAction action) const {
		return _hotkeys[(size_t)action];
	}

	void SetHotkey(HotkeyAction action, Magpie::App::HotkeySettings const& value);

	event_token HotkeyChanged(EventHandler<HotkeyAction> const& handler) {
		return _hotkeyChangedEvent.add(handler);
	}

	void HotkeyChanged(event_token const& token) {
		_hotkeyChangedEvent.remove(token);
	}

	bool IsAutoRestore() const noexcept {
		return _isAutoRestore;
	}

	void IsAutoRestore(bool value) noexcept;

	event_token IsAutoRestoreChanged(EventHandler<bool> const& handler) {
		return _isAutoRestoreChangedEvent.add(handler);
	}

	void IsAutoRestoreChanged(event_token const& token) {
		_isAutoRestoreChangedEvent.remove(token);
	}

	uint32_t DownCount() const noexcept {
		return _downCount;
	}

	void DownCount(uint32_t value) noexcept;

	event_token DownCountChanged(EventHandler<uint32_t> const& handler) {
		return _downCountChangedEvent.add(handler);
	}

	void DownCountChanged(event_token const& token) {
		_downCountChangedEvent.remove(token);
	}

	bool IsDeveloperMode() const noexcept {
		return _isDeveloperMode;
	}

	void IsDeveloperMode(bool value) noexcept;

	event_token IsDeveloperModeChanged(EventHandler<bool> const& handler) {
		return _isDeveloperModeChangedEvent.add(handler);
	}

	void IsDeveloperModeChanged(event_token const& token) {
		_isDeveloperModeChangedEvent.remove(token);
	}

	bool IsBreakpointMode() const noexcept {
		return _isBreakpointMode;
	}

	void IsBreakpointMode(bool value) noexcept {
		_isBreakpointMode = value;
	}

	Magpie::Runtime::MagSettings GetMagSettings(uint64_t hWnd);

private:
	bool _LoadSettings(std::string text);
	void _SetDefaultHotkeys();

	bool _isPortableMode = false;
	hstring _workingDir;

	// 0: 浅色
	// 1: 深色
	// 2: 系统
	int _theme = 2;
	event<EventHandler<int>> _themeChangedEvent;

	Rect _windowRect{ CW_USEDEFAULT,CW_USEDEFAULT,1280,820 };
	bool _isWindowMaximized = false;

	std::array<Magpie::App::HotkeySettings, (size_t)HotkeyAction::COUNT_OR_NONE> _hotkeys;
	event<EventHandler<HotkeyAction>> _hotkeyChangedEvent;

	bool _isAutoRestore = false;
	event<EventHandler<bool>> _isAutoRestoreChangedEvent;

	uint32_t _downCount = 5;
	event<EventHandler<uint32_t>> _downCountChangedEvent;

	bool _isDeveloperMode = false;
	event<EventHandler<bool>> _isDeveloperModeChangedEvent;

	bool _isBreakpointMode = false;

	Magpie::Runtime::MagSettings _defaultMagSettings;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct Settings : SettingsT<Settings, implementation::Settings> {
};

}
