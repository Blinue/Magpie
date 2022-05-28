#pragma once

#include "DummyTemplate.g.h"

namespace winrt::Magpie::implementation
{
    struct DummyTemplate : DummyTemplate_base<DummyTemplate>
    {
        DummyTemplate();
    };
}

namespace winrt::Magpie::factory_implementation
{
    struct DummyTemplate : DummyTemplateT<DummyTemplate, implementation::DummyTemplate>
    {
    };
}
