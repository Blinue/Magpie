#pragma once

#include "Controls.Setting.g.h"

namespace winrt::Magpie::App::Controls::implementation
{
    struct Setting : SettingT<Setting>
    {
        Setting() = default;
    };
}

namespace winrt::Magpie::App::Controls::factory_implementation
{
    struct Setting : SettingT<Setting, implementation::Setting>
    {
    };
}
