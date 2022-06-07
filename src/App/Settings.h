#pragma once
#include "pch.h"
#include "Settings.g.h"


namespace winrt::Magpie::implementation {

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
	winrt::event_token ThemeChanged(Windows::Foundation::EventHandler<int> const& handler);
	void ThemeChanged(winrt::event_token const& token);

	Windows::Foundation::Rect WindowRect() const noexcept {
		return _windowRect;
	}

	void WindowRect(const Windows::Foundation::Rect& value) noexcept {
		_windowRect = value;
	}

	bool IsWindowMaximized() const noexcept {
		return _isWindowMaximized;
	}

	void IsWindowMaximized(bool value) noexcept {
		_isWindowMaximized = value;
	}

	bool IsPaneOpen() const noexcept {
		return _isPaneOpen;
	}

	void IsPaneOpen(bool value) noexcept {
		_isPaneOpen = value;
	}

private:
	bool _isPortableMode = false;
	hstring _workingDir;

	// 0: 浅色
	// 1: 深色
	// 2: 系统
	int _theme = 2;
	event<Windows::Foundation::EventHandler<int>> _themeChangedEvent;

	Windows::Foundation::Rect _windowRect{ CW_USEDEFAULT,CW_USEDEFAULT,1280,820 };
	bool _isWindowMaximized = false;
	bool _isPaneOpen = true;
};

}

namespace winrt::Magpie::factory_implementation {

struct Settings : SettingsT<Settings, implementation::Settings> {
};

}
