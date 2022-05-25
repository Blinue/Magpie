#pragma once
#include "pch.h"
#include "Settings.g.h"


namespace winrt::Magpie::App::implementation {

struct Settings : SettingsT<Settings> {
	Settings() = default;

	bool Initialize(uint64_t pLogger);

	bool Save();

	hstring WorkingDir() const {
		return _workingDir;
	}

	bool IsPortableMode() const {
		return _isPortableMode;
	}

	void IsPortableMode(bool value);

	int Theme() const {
		return _theme;
	}
	void Theme(int value);
	winrt::event_token ThemeChanged(Windows::Foundation::EventHandler<int> const& handler);
	void ThemeChanged(winrt::event_token const& token) noexcept;

private:
	bool _isPortableMode = false;
	hstring _workingDir;

	// 0: 浅色
	// 1: 深色
	// 2: 系统
	int _theme = 2;
	event<Windows::Foundation::EventHandler<int>> _themeChangedEvent;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct Settings : SettingsT<Settings, implementation::Settings> {
};

}
