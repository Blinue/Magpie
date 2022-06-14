#pragma once

#include "DummyTemplate.g.h"

namespace winrt::Magpie::App::implementation
{
    struct DummyTemplate : DummyTemplate_base<DummyTemplate>
    {
        DummyTemplate();
    };
}

namespace winrt::Magpie::App::factory_implementation
{
    struct DummyTemplate : DummyTemplateT<DummyTemplate, implementation::DummyTemplate>
    {
    };
}
