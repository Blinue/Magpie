#include "pch.h"
#include "ScalingModeItem.h"
#if __has_include("ScalingModeItem.g.cpp")
#include "ScalingModeItem.g.cpp"
#endif
#include "ScalingMode.h"
#include "ScalingModesService.h"

using namespace Magpie::Core;


namespace winrt::Magpie::UI::implementation {

ScalingModeItem::ScalingModeItem(uint32_t index) {
	_scalingMode = &ScalingModesService::Get().GetScalingMode(index);

	std::vector<IInspectable> effects;
	for (auto& effect : _scalingMode->effects) {
		std::wstring_view name = effect.name;

		size_t delimPos = effect.name.find_last_of(L'\\');
		if (delimPos != std::wstring::npos) {
			effects.push_back(box_value(name.substr(delimPos + 1)));
		} else {
			effects.push_back(box_value(name));
		}
	}
	_effects = winrt::single_threaded_observable_vector(std::move(effects));
}

hstring ScalingModeItem::Name() const noexcept {
	return hstring(_scalingMode->name);
}

void ScalingModeItem::Name(const hstring& value) noexcept {
	_scalingMode->name = value;
}

hstring ScalingModeItem::Description() const noexcept {
	std::wstring result;
	for (const EffectOption& effect : _scalingMode->effects) {
		if (!result.empty()) {
			result.append(L" > ");
		}

		size_t delimPos = effect.name.find_last_of(L'\\');
		if (delimPos == std::wstring::npos) {
			result += effect.name;
		} else {
			result += effect.name.substr(delimPos + 1);
		}
	}
	return hstring(result);
}

}
