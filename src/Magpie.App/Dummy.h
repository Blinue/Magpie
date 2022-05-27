#pragma once

#include "Dummy.g.h"

namespace winrt::Magpie::App::implementation
{
    struct Dummy : Dummy_base<Dummy>
    {
        Dummy();
    };
}

namespace winrt::Magpie::App::factory_implementation
{
    struct Dummy : DummyT<Dummy, implementation::Dummy>
    {
    };
}
