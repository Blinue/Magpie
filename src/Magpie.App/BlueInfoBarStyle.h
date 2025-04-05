#pragma once
#include "BlueInfoBarStyle.g.h"

namespace winrt::Magpie::App::implementation {
struct BlueInfoBarStyle : BlueInfoBarStyleT<BlueInfoBarStyle> {};
}

namespace winrt::Magpie::App::factory_implementation {
struct BlueInfoBarStyle : BlueInfoBarStyleT<BlueInfoBarStyle, implementation::BlueInfoBarStyle> {};
}
