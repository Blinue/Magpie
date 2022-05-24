#pragma once

#include "Settings.g.h"

namespace winrt::Magpie::App::implementation {

struct Settings : SettingsT<Settings> {
	Settings();

	bool IsPortableMode() const;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct Settings : SettingsT<Settings, implementation::Settings> {
};

}
