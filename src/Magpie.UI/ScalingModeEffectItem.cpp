#include "pch.h"
#include "ScalingModeEffectItem.h"
#if __has_include("ScalingModeEffectItem.g.cpp")
#include "ScalingModeEffectItem.g.cpp"
#endif
#include <Magpie.Core.h>
#include "ScalingModesService.h"
#include "EffectsService.h"


namespace Core = ::Magpie::Core;
using namespace Magpie::Core;


static std::wstring_view GetEffectDisplayName(std::wstring_view fullName) {
	size_t delimPos = fullName.find_last_of(L'\\');
	return delimPos != std::wstring::npos ? fullName.substr(delimPos + 1) : fullName;
}

namespace winrt::Magpie::UI::implementation {

ScalingModeEffectItem::ScalingModeEffectItem(uint32_t scalingModeIdx, uint32_t effectIdx) 
	: _scalingModeIdx(scalingModeIdx), _effectIdx(effectIdx)
{
	const std::wstring& name = _Data().name;
	_name = GetEffectDisplayName(name);
	_effectInfo = EffectsService::Get().GetEffect(name);
}

bool ScalingModeEffectItem::CanScale() const noexcept {
	return _effectInfo && _effectInfo->canScale;
}

bool ScalingModeEffectItem::HasParameters() const noexcept {
	return _effectInfo && !_effectInfo->params.empty();
}

int ScalingModeEffectItem::ScalingType() const noexcept {
	return (int)_Data().scalingType;
}

void ScalingModeEffectItem::ScalingType(int value) {
	if (value < 0) {
		return;
	}

	EffectOption& data = _Data();
	Core::ScalingType scalingType = (Core::ScalingType)value;
	if (data.scalingType == scalingType) {
		return;
	}

	data.scalingType = scalingType;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingType"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsShowScalingFactors"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsShowScalingPixels"));
}

bool ScalingModeEffectItem::IsShowScalingFactors() const noexcept {
	Core::ScalingType scalingType = _Data().scalingType;
	return scalingType == Core::ScalingType::Normal || scalingType == Core::ScalingType::Fit;
}

bool ScalingModeEffectItem::IsShowScalingPixels() const noexcept {
	return _Data().scalingType == Core::ScalingType::Absolute;
}

double ScalingModeEffectItem::ScalingFactorX() const noexcept {
	return _Data().scale.first;
}

void ScalingModeEffectItem::ScalingFactorX(double value) {
	_Data().scale.first = (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingFactorX"));
}

double ScalingModeEffectItem::ScalingFactorY() const noexcept {
	return _Data().scale.second;
}

void ScalingModeEffectItem::ScalingFactorY(double value) {
	_Data().scale.second = (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingFactorX"));
}

void ScalingModeEffectItem::Remove() {
	_removedEvent(*this, _effectIdx);
}

EffectOption& ScalingModeEffectItem::_Data() noexcept {
	return ScalingModesService::Get().GetScalingMode(_scalingModeIdx).effects[_effectIdx];
}

const EffectOption& ScalingModeEffectItem::_Data() const noexcept {
	return ScalingModesService::Get().GetScalingMode(_scalingModeIdx).effects[_effectIdx];
}

}
