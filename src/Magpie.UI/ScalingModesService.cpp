#include "pch.h"
#include "ScalingModesService.h"
#include "AppSettings.h"


namespace winrt::Magpie::UI {

ScalingMode& ScalingModesService::GetScalingMode(uint32_t idx) {
    return AppSettings::Get().ScalingModes()[idx];
}

uint32_t ScalingModesService::GetScalingModeCount() {
    return (uint32_t)AppSettings::Get().ScalingModes().size();
}

void ScalingModesService::AddScalingMode(std::wstring_view name, int copyFrom) {
    assert(!name.empty());

    std::vector<ScalingMode>& scalingModes = AppSettings::Get().ScalingModes();
    if (copyFrom < 0) {
        scalingModes.emplace_back().name = name;
    } else {
        scalingModes.emplace_back(scalingModes[copyFrom]).name = name;
    }
}

}
