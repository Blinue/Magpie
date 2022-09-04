#pragma once
#include "pch.h"
#include <Magpie.Core.h>


namespace winrt::Magpie::UI {

struct ScalingMode {
	std::wstring name;
	std::vector<::Magpie::Core::EffectOption> effects;
};

}
