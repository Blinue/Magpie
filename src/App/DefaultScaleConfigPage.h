#pragma once

#include "DefaultScaleConfigPage.g.h"


namespace winrt::Magpie::App::implementation {

struct DefaultScaleConfigPage : DefaultScaleConfigPageT<DefaultScaleConfigPage> {
    DefaultScaleConfigPage();
};

}

namespace winrt::Magpie::App::factory_implementation {

struct DefaultScaleConfigPage : DefaultScaleConfigPageT<DefaultScaleConfigPage, implementation::DefaultScaleConfigPage> {
};

}
