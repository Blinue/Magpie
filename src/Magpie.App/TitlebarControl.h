#pragma once
#include "TitleBarControl.g.h"

namespace winrt::Magpie::App::implementation {
struct TitleBarControl : TitleBarControlT<TitleBarControl> {
    TitleBarControl() = default;
};
}

namespace winrt::Magpie::App::factory_implementation {

struct TitleBarControl : TitleBarControlT<TitleBarControl, implementation::TitleBarControl> {
};

}
