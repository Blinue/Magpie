#pragma once

#include "ScaleModesPage.g.h"

namespace winrt::Magpie::App::implementation {

struct ScaleModesPage : ScaleModesPageT<ScaleModesPage> {
    ScaleModesPage();
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ScaleModesPage : ScaleModesPageT<ScaleModesPage, implementation::ScaleModesPage> {
};

}
