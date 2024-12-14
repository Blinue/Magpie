#pragma once
#include "ScalingOptions.h"

namespace Magpie {

struct ScalingMode {
	std::wstring name;
	std::vector<::Magpie::Core::EffectOption> effects;
};

}
