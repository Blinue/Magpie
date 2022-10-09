#pragma once
#include "pch.h"
#include <winrt/Magpie.UI.h>


namespace winrt::Magpie::UI {

struct HotkeyHelper {
	static std::string ToString(winrt::Magpie::UI::HotkeyAction action);

	static bool IsValidKeyCode(DWORD code);

	static DWORD StringToKeyCode(std::wstring_view str);
};

}

namespace winrt {

// 将 HotkeyAction 映射为字符串
hstring to_hstring(Magpie::UI::HotkeyAction action);

}
