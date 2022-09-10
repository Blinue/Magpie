#include "pch.h"
#include "ScalingModesViewModel.h"
#if __has_include("ScalingModesViewModel.g.cpp")
#include "ScalingModesViewModel.g.cpp"
#endif
#include "ScalingModesService.h"


namespace winrt::Magpie::UI::implementation {

ScalingModesViewModel::ScalingModesViewModel() {
	std::vector<IInspectable> scalingModes;
	for (uint32_t i = 0, count = ScalingModesService::Get().GetScalingModeCount(); i < count;++i) {
		scalingModes.push_back(ScalingModeItem(i));
	}
	_scalingModes = winrt::single_threaded_observable_vector(std::move(scalingModes));
}

}
