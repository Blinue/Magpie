#pragma once
#include "TitlebarControl.g.h"

namespace winrt::Magpie::App::implementation {
struct TitlebarControl : TitlebarControlT<TitlebarControl> {
    TitlebarControl() {
        
    }
};
}

namespace winrt::Magpie::App::factory_implementation {

struct TitlebarControl : TitlebarControlT<TitlebarControl, implementation::TitlebarControl> {
};

}
