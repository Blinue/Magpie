#include "pch.h"
#include "ScalingMode.h"
#include "StrHelper.h"

namespace Magpie {

EffectItem::operator EffectOption()  const noexcept {
	EffectOption result = {
		.name = StrHelper::UTF16ToUTF8(name),
		.scalingType = scalingType,
		.scale = scale
	};

	result.parameters.reserve(parameters.size());
	for (const auto& pair : parameters) {
		result.parameters.emplace(StrHelper::UTF16ToUTF8(pair.first), pair.second);
	}

	return result;
}

}
