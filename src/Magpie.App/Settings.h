#pragma once
#include "pch.h"
#include "Settings.g.h"


namespace winrt::Magpie::App::implementation {

struct Settings : SettingsT<Settings> {
	Settings() = default;

	bool Initialize();

	bool IsPortableMode() const {
		return _isPortableMode;
	}
	void IsPortableMode(bool value) {
		_isPortableMode = value;
	}

	bool Save();

private:
	bool _isPortableMode = false;
	// 0: 系统
	// 1: 浅色
	// 2: 深色
	uint32_t _theme = 0;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct Settings : SettingsT<Settings, implementation::Settings> {
};

}
