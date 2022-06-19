#pragma once
#include "pch.h"
#include "Settings.g.h"


namespace winrt::Magpie::App::implementation {

struct Settings : SettingsT<Settings> {
	Settings() = default;

	bool Initialize(uint64_t pLogger);

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
	event_token ThemeChanged(EventHandler<int> const& handler);
	void ThemeChanged(event_token const& token);

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

	Magpie::App::HotkeySettings GetHotkey(HotkeyAction action) const;
	void SetHotkey(HotkeyAction action, Magpie::App::HotkeySettings const& value);

	event_token HotkeyChanged(EventHandler<HotkeyAction> const& handler);
	void HotkeyChanged(event_token const& token);

private:
	bool _isPortableMode = false;
	hstring _workingDir;

	// 0: 浅色
	// 1: 深色
	// 2: 系统
	int _theme = 2;
	event<EventHandler<int>> _themeChangedEvent;

	Rect _windowRect{ CW_USEDEFAULT,CW_USEDEFAULT,1280,820 };
	bool _isWindowMaximized = false;

	std::array<Magpie::App::HotkeySettings, (size_t)HotkeyAction::COUNT> _hotkeys;
	event<EventHandler<HotkeyAction>> _hotkeyChangedEvent;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct Settings : SettingsT<Settings, implementation::Settings> {
};

}
