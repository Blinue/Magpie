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
			std::abs(scale.first - 1.0f) > 1e-5 || std::abs(scale.second - 1.0f) > 1e-5;
	}

	explicit operator EffectOption() const noexcept;
};

struct ScalingMode {
	std::wstring name;
	std::vector<EffectItem> effects;
};

}
