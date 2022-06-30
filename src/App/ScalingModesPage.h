#pragma once

#include "ScalingModesPage.g.h"

namespace winrt::Magpie::App::implementation {

struct ScalingModesPage : ScalingModesPageT<ScalingModesPage> {
	ScalingModesPage();
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ScalingModesPage : ScalingModesPageT<ScalingModesPage, implementation::ScalingModesPage> {
};

}
