#pragma once
#include <winrt/Magpie.App.h>

namespace winrt::Magpie::App {

struct HotkeyHelper {
	static std::string ToString(winrt::Magpie::App::HotkeyAction action);

	static bool IsValidKeyCode(uint8_t code);
};

}

namespace winrt {

// 将 HotkeyAction 映射为字符串
hstring to_hstring(Magpie::App::HotkeyAction action);

}
