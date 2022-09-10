#include "pch.h"
#include "ScalingModeItem.h"
#if __has_include("ScalingModeItem.g.cpp")
#include "ScalingModeItem.g.cpp"
#endif
#include "ScalingMode.h"
#include "ScalingModesService.h"


namespace winrt::Magpie::UI::implementation {

ScalingModeItem::ScalingModeItem(uint32_t index) {
	_scalingMode = &ScalingModesService::Get().GetScalingMode(index);
}

hstring ScalingModeItem::Name() const noexcept {
	return hstring(_scalingMode->name);
}

void ScalingModeItem::Name(const hstring& value) noexcept {
	_scalingMode->name = value;
}

}
