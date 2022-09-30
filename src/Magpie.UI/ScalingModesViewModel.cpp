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

	_scalingModeMovedRevoker = ScalingModesService::Get().ScalingModeMoved(
		auto_revoke, { this, &ScalingModesViewModel::_ScalingModesService_Reordered });
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
	ScalingModesService::Get().AddScalingMode(_newScalingModeName, _newScalingModeCopyFrom - 1);
	_scalingModes.Append(ScalingModeItem(ScalingModesService::Get().GetScalingModeCount() - 1));
}

void ScalingModesViewModel::_ScalingModesService_Reordered(uint32_t index, bool isMoveUp) {
	uint32_t targetIndex = isMoveUp ? index - 1 : index + 1;
	_scalingModes.GetAt(index).as<ScalingModeItem>().Index(targetIndex);

	ScalingModeItem targetItem = _scalingModes.GetAt(targetIndex).as<ScalingModeItem>();
	targetItem.Index(index);
	
	_scalingModes.RemoveAt(targetIndex);
	_scalingModes.InsertAt(index, targetItem);
}

}
