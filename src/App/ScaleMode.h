#pragma once
#include "pch.h"
#include <Runtime.h>


namespace winrt::Magpie::App {

struct ScaleMode {
	std::wstring Name;
	std::vector<::Magpie::Runtime::EffectOption> Effects;
};

}
