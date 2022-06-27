#pragma once

#include "ScalingConfigPage.g.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App::implementation {

struct ScalingConfigPage : ScalingConfigPageT<ScalingConfigPage> {
    ScalingConfigPage();

    void CaptureModeComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const& args);

private:
    Magpie::Runtime::MagSettings _magSettings{ nullptr };
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ScalingConfigPage : ScalingConfigPageT<ScalingConfigPage, implementation::ScalingConfigPage> {
};

}
