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

}
