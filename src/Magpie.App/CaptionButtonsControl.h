#pragma once
#include "CaptionButtonsControl.g.h"

namespace winrt::Magpie::App::implementation {

struct CaptionButtonsControl : CaptionButtonsControlT<CaptionButtonsControl> {
    CaptionButtonsControl() {

    }

};

}

namespace winrt::Magpie::App::factory_implementation {

struct CaptionButtonsControl : CaptionButtonsControlT<CaptionButtonsControl, implementation::CaptionButtonsControl> {
};

}
