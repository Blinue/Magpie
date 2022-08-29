#pragma once
#include "pch.h"
#include <Magpie.Core.h>


namespace winrt::Magpie::App {

struct ScaleMode {
	std::wstring name;
	std::vector<::Magpie::Core::EffectOption> effects;
};

}
