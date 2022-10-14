#pragma once
#include "pch.h"


namespace winrt::Magpie::UI {

struct EffectHelper {
	static std::wstring_view GetDisplayName(std::wstring_view fullName) noexcept {
		size_t delimPos = fullName.find_last_of(L'\\');
		return delimPos != std::wstring::npos ? fullName.substr(delimPos + 1) : fullName;
	}
};

}
