#pragma once
#include <Magpie.Core.h>

namespace winrt::Magpie::App {

struct ScalingMode {
	std::wstring name;
	std::vector<::Magpie::Core::EffectOption> effects;
};

}
