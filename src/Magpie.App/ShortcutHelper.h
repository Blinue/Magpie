#pragma once
#include <winrt/Magpie.App.h>
#include "Shortcut.h"

namespace winrt::Magpie::App {

struct ShortcutHelper {
	static std::string ToString(winrt::Magpie::App::ShortcutAction action) noexcept;

	static bool IsValidKeyCode(uint8_t code) noexcept;

	static ShortcutError CheckShortcut(Shortcut shortcut) noexcept;
};

}

namespace winrt {

// 将 ShortcutAction 映射为字符串
hstring to_hstring(Magpie::App::ShortcutAction action);

}
