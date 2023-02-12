#include "pch.h"
#include "HotkeyHelper.h"
#include "Win32Utils.h"
#include <parallel_hashmap/phmap.h>


namespace winrt::Magpie::App {

std::string HotkeyHelper::ToString(winrt::Magpie::App::ShortcutAction action) {
	using winrt::Magpie::App::ShortcutAction;

	switch (action) {
	case ShortcutAction::Scale:
		return "Scale";
	case ShortcutAction::Overlay:
		return "Overlay";
	case ShortcutAction::COUNT_OR_NONE:
		return "None";
	default:
		break;
	}

	return {};
}

bool HotkeyHelper::IsValidKeyCode(uint8_t code) {
	static phmap::flat_hash_set<uint8_t> validKeyCodes = []() {
		phmap::flat_hash_set<uint8_t> keyCodes;
		keyCodes.reserve(99);

		// 字母
		for (uint8_t i = 'A'; i <= 'Z'; ++i) {
			keyCodes.insert(i);
		}

		// 数字（顶部）
		for (uint8_t i = '0'; i <= '9'; ++i) {
			keyCodes.insert(i);
		}

		// 数字（小键盘）
		for (uint8_t i = VK_NUMPAD0; i <= VK_NUMPAD9; ++i) {
			keyCodes.insert(i);
		}

		// F1~F24
		for (uint8_t i = VK_F1; i <= VK_F24; ++i) {
			keyCodes.insert(i);
		}

		// 空格、Page Up/Down、End、Home、方向键
		for (uint8_t i = VK_SPACE; i <= VK_DOWN; ++i) {
			keyCodes.insert(i);
		}

		// 分号、等号、逗号、-、句号、/、`
		for (uint8_t i = VK_OEM_1; i <= VK_OEM_3; ++i) {
			keyCodes.insert(i);
		}

		// [、\、]、'
		for (uint8_t i = VK_OEM_4; i <= VK_OEM_7; ++i) {
			keyCodes.insert(i);
		}

		keyCodes.insert((uint8_t)VK_INSERT);	// Insert
		keyCodes.insert((uint8_t)VK_DELETE);	// Delete
		keyCodes.insert((uint8_t)VK_ADD);		// 加（小键盘）
		keyCodes.insert((uint8_t)VK_SUBTRACT);	// 减（小键盘）
		keyCodes.insert((uint8_t)VK_MULTIPLY);	// 乘（小键盘）
		keyCodes.insert((uint8_t)VK_DIVIDE);	// 除（小键盘）
		keyCodes.insert((uint8_t)VK_DECIMAL);	// .（小键盘）
		keyCodes.insert((uint8_t)VK_BACK);		// Backspace
		keyCodes.insert((uint8_t)VK_RETURN);	// 回车

		return keyCodes;
	}();

	return validKeyCodes.contains(code);
}

} // namespace winrt::Magpie::App

namespace winrt {

using Magpie::App::ShortcutAction;

hstring to_hstring(ShortcutAction action) {
	switch (action) {
	case ShortcutAction::Scale:
		return L"Scale";
	case ShortcutAction::Overlay:
		return L"Overlay";
	case ShortcutAction::COUNT_OR_NONE:
		return L"None";
	default:
		break;
	}

	return {};
}

} // namespace winrt
