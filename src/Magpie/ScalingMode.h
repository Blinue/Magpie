#pragma once
#include "ScalingOptions.h"

namespace Magpie {

struct EffectItem {
	std::wstring name;
	phmap::flat_hash_map<std::wstring, float> parameters;
	ScalingType scalingType = ScalingType::Normal;
	std::pair<float, float> scale = { 1.0f,1.0f };

	bool HasScale() const noexcept {
		return scalingType != ScalingType::Normal ||
			!IsApprox(scale.first, 1.0f) || !IsApprox(scale.second, 1.0f);
	}

	explicit operator EffectOption() const noexcept;
};

struct ScalingMode {
	std::wstring name;
	std::vector<EffectItem> effects;
};

}
