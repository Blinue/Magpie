#pragma once
#include "pch.h"
#include "Settings.g.h"


namespace winrt::Magpie::App::implementation {

struct Settings : SettingsT<Settings> {
	Settings() = default;

	bool Initialize(const hstring& workingDir);

	bool Save();

	hstring WorkingDir() const {
		return _workingDir;
	}

	static bool IsPortableMode();

private:
	hstring _workingDir;

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
