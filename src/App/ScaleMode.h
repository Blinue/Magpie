#pragma once
#include "pch.h"
#include <Runtime.h>


namespace winrt::Magpie::App {

struct ScaleMode {
	std::wstring name;
	std::vector<::Magpie::Runtime::EffectOption> effects;
};

}
