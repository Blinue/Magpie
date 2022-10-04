#include "pch.h"
#include "ScalingModeEffectItem.h"
#if __has_include("ScalingModeEffectItem.g.cpp")
#include "ScalingModeEffectItem.g.cpp"
#endif
#include <Magpie.Core.h>
#include "ScalingModesService.h"
#include "EffectsService.h"


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
