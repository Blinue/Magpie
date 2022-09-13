#include "pch.h"
#include "ScalingModesViewModel.h"
#if __has_include("ScalingModesViewModel.g.cpp")
#include "ScalingModesViewModel.g.cpp"
#endif
#include "ScalingModesService.h"
#include "AppSettings.h"


namespace winrt::Magpie::UI::implementation {

ScalingModesViewModel::ScalingModesViewModel() {
	std::vector<IInspectable> scalingModes;
	for (uint32_t i = 0, count = ScalingModesService::Get().GetScalingModeCount(); i < count;++i) {
		scalingModes.push_back(ScalingModeItem(i));
	}
	_scalingModes = single_threaded_observable_vector(std::move(scalingModes));
}

void ScalingModesViewModel::PrepareForAdd() {
	std::vector<IInspectable> copyFromList;
	copyFromList.push_back(box_value(L"无"));
	for (const auto& scalingMode : AppSettings::Get().ScalingModes()) {
		copyFromList.push_back(box_value(scalingMode.name));
	}
	_newScalingModeCopyFromList = single_threaded_vector(std::move(copyFromList));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"NewScalingModeCopyFromList"));

	_newScalingModeName.clear();
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"NewScalingModeName"));

	_newScalingModeCopyFrom = 0;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"NewScalingModeCopyFrom"));
}

void ScalingModesViewModel::NewScalingModeName(const hstring& value) noexcept {
	_newScalingModeName = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"NewScalingModeName"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsAddButtonEnabled"));
}

void ScalingModesViewModel::AddScalingMode() {

}

}
