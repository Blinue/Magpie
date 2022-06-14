#pragma once

#include "HotkeyManager.g.h"


namespace winrt::Magpie::App::implementation {

struct HotkeyManager : HotkeyManagerT<HotkeyManager> {
	HotkeyManager() = default;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HotkeyManager : HotkeyManagerT<HotkeyManager, implementation::HotkeyManager> {
};

}
