#pragma once
#include <winrt/Magpie.h>
#include "Shortcut.h"

namespace Magpie {

struct ShortcutHelper {
	static std::string ToString(winrt::Magpie::ShortcutAction action) noexcept;

	static bool IsValidKeyCode(uint8_t code) noexcept;

	static winrt::Magpie::ShortcutError CheckShortcut(Shortcut shortcut) noexcept;
};

}

namespace winrt {

// 将 ShortcutAction 映射为字符串
hstring to_hstring(Magpie::ShortcutAction action);

}
