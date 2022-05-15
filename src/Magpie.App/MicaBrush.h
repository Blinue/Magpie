#pragma once

#include "MicaBrush.g.h"

namespace winrt::Magpie::App::implementation
{
    struct MicaBrush : MicaBrushT<MicaBrush>
    {
        MicaBrush() = default;
    };
}

namespace winrt::Magpie::App::factory_implementation
{
    struct MicaBrush : MicaBrushT<MicaBrush, implementation::MicaBrush>
    {
    };
}
