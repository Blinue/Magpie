#pragma once

#include "DefaultScalingConfigPage.g.h"


namespace winrt::Magpie::App::implementation {

struct DefaultScalingConfigPage : DefaultScalingConfigPageT<DefaultScalingConfigPage> {
    DefaultScalingConfigPage();
};

}

namespace winrt::Magpie::App::factory_implementation {

struct DefaultScalingConfigPage : DefaultScalingConfigPageT<DefaultScalingConfigPage, implementation::DefaultScalingConfigPage> {
};

}
