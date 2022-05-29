#pragma once

#include "PageFrame.g.h"


namespace winrt::Magpie::implementation {

struct PageFrame : PageFrame_base<PageFrame> {
    PageFrame() = default;
};

}

namespace winrt::Magpie::factory_implementation {

struct PageFrame : PageFrameT<PageFrame, implementation::PageFrame> {
};

}
