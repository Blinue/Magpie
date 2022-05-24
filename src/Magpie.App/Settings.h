#pragma once

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

private:
	bool _isPortableMode = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct Settings : SettingsT<Settings, implementation::Settings> {
};

}
