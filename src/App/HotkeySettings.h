#pragma once

#include "HotkeySettings.g.h"

namespace winrt::Magpie::implementation {

struct HotkeySettings : HotkeySettingsT<HotkeySettings> {
	HotkeySettings() = default;

};

}

namespace winrt::Magpie::factory_implementation {

struct HotkeySettings : HotkeySettingsT<HotkeySettings, implementation::HotkeySettings> {
};

}
