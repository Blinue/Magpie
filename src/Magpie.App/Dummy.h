#pragma once

#include "Controls.Dummy.g.h"

namespace winrt::Magpie::App::Controls::implementation
{
    struct Dummy : DummyT<Dummy>
    {
        Dummy();
    };
}

namespace winrt::Magpie::App::Controls::factory_implementation
{
    struct Dummy : DummyT<Dummy, implementation::Dummy>
    {
    };
}
