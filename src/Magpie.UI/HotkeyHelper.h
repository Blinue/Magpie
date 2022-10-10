#pragma once
#include "pch.h"
#include <winrt/Magpie.UI.h>


namespace winrt::Magpie::UI {

struct HotkeyHelper {
	static std::string ToString(winrt::Magpie::UI::HotkeyAction action);

	static bool IsValidKeyCode(uint8_t code);
};

}

namespace winrt {

// 将 HotkeyAction 映射为字符串
hstring to_hstring(Magpie::UI::HotkeyAction action);

}
