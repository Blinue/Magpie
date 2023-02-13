#pragma once
#include "WrapPanel.g.h"

namespace winrt::Magpie::App::implementation {

struct WrapPanel : WrapPanelT<WrapPanel> {
    WrapPanel() = default;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct WrapPanel : WrapPanelT<WrapPanel, implementation::WrapPanel> {
};

}
