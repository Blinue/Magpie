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

bool ScalingModesService::Reorder(uint32_t scalingModeIdx, bool isMoveUp) {
    std::vector<ScalingMode>& profiles = AppSettings::Get().ScalingModes();
    if (isMoveUp ? scalingModeIdx == 0 : scalingModeIdx + 1 >= (uint32_t)profiles.size()) {
        return false;
    }

    int targetIdx = isMoveUp ? (int)scalingModeIdx - 1 : (int)scalingModeIdx + 1;
    std::swap(profiles[scalingModeIdx], profiles[targetIdx]);
    for (ScalingProfile& profile : AppSettings::Get().ScalingProfiles()) {
        if (profile.scalingMode == (int)scalingModeIdx) {
            profile.scalingMode = targetIdx;
        } else if (profile.scalingMode == targetIdx) {
            profile.scalingMode = scalingModeIdx;
        }
    }

    _reorderedEvent(scalingModeIdx, isMoveUp);
    return true;
}

}
