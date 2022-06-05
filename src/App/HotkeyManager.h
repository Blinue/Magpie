#pragma once

#include "HotkeyManager.g.h"


namespace winrt::Magpie::implementation {

struct HotkeyManager : HotkeyManagerT<HotkeyManager> {
	HotkeyManager() = default;
};

}

namespace winrt::Magpie::factory_implementation {

struct HotkeyManager : HotkeyManagerT<HotkeyManager, implementation::HotkeyManager> {
};

}
