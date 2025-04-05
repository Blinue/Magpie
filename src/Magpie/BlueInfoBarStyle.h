#pragma once
#include "BlueInfoBarStyle.g.h"

namespace winrt::Magpie::implementation {
struct BlueInfoBarStyle : BlueInfoBarStyleT<BlueInfoBarStyle> {};
}

namespace winrt::Magpie::factory_implementation {
struct BlueInfoBarStyle : BlueInfoBarStyleT<BlueInfoBarStyle, implementation::BlueInfoBarStyle> {};
}
