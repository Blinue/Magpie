#include "pch.h"
#include "ShortcutHelper.h"
#include "Win32Helper.h"
#include <bitset>

using namespace winrt::Magpie;

namespace Magpie {

std::string ShortcutHelper::ToString(ShortcutAction action) noexcept {
	switch (action) {
	case ShortcutAction::Scale:
		return "Scale";
	case ShortcutAction::WindowedModeScale:
		return "WindowedModeScale";
	case ShortcutAction::Overlay:
		return "Overlay";
	case ShortcutAction::COUNT_OR_NONE:
		return "None";
	default:
		break;
	}

	return {};
}

bool ShortcutHelper::IsValidKeyCode(uint8_t code) noexcept {
	// C++23 可编译时求值
	static std::bitset<256> validKeyCodes = []() {
		std::bitset<256> result;

		// 字母
		for (uint8_t i = 'A'; i <= 'Z'; ++i) {
			result[i] = true;
		}

		// 数字（顶部）
		for (uint8_t i = '0'; i <= '9'; ++i) {
			result[i] = true;
		}

		// 数字（小键盘）
		for (uint8_t i = VK_NUMPAD0; i <= VK_NUMPAD9; ++i) {
			result[i] = true;
		}

		// F1~F24
		for (uint8_t i = VK_F1; i <= VK_F24; ++i) {
			result[i] = true;
		}

		// 空格、Page Up/Down、End、Home、方向键
		for (uint8_t i = VK_SPACE; i <= VK_DOWN; ++i) {
			result[i] = true;
		}

		// 分号、等号、逗号、-、句号、/、`
		for (uint8_t i = VK_OEM_1; i <= VK_OEM_3; ++i) {
			result[i] = true;
		}

		// [、\、]、'
		for (uint8_t i = VK_OEM_4; i <= VK_OEM_7; ++i) {
			result[i] = true;
		}

		result[VK_INSERT] = true;
		result[VK_DELETE] = true;
		result[VK_ADD] = true;		// 加（小键盘）
		result[VK_SUBTRACT] = true;	// 减（小键盘）
		result[VK_MULTIPLY] = true;	// 乘（小键盘）
		result[VK_DIVIDE] = true;	// 除（小键盘）
		result[VK_DECIMAL] = true;	// .（小键盘）
		result[VK_BACK] = true;		// Backspace
		result[VK_RETURN] = true;	// 回车
		result[VK_TAB] = true;
		result[VK_SNAPSHOT] = true;	// Print Screen
		result[VK_PAUSE] = true;
		result[VK_CANCEL] = true;	// Break（即 Ctrl+Pause）

		return result;
	}();

	return validKeyCodes[code];
}

ShortcutError ShortcutHelper::CheckShortcut(Shortcut shortcut) noexcept {
	UINT modifiers = MOD_NOREPEAT;
	UINT modCount = 0;

	if (shortcut.win) {
		++modCount;
		modifiers |= MOD_WIN;
	}
	if (shortcut.ctrl) {
		++modCount;
		modifiers |= MOD_CONTROL;
	}
	if (shortcut.alt) {
		++modCount;
		modifiers |= MOD_ALT;
	}
	if (shortcut.shift) {
		++modCount;
		modifiers |= MOD_SHIFT;
	}

	if (modCount == 0 || (modCount == 1 && shortcut.code == 0)) {
		// 必须存在 Modifier
		// 如果只有一个 Modifier 则必须存在 Virtual Key
		return ShortcutError::Invalid;
	}

	// 检测快捷键是否被占用
	if (!RegisterHotKey(NULL, (int)ShortcutAction::COUNT_OR_NONE, modifiers, shortcut.code)) {
		return ShortcutError::InUse;
	}

	UnregisterHotKey(NULL, (int)ShortcutAction::COUNT_OR_NONE);
	return ShortcutError::NoError;
}

} // namespace winrt::Magpie

namespace winrt {

using Magpie::ShortcutAction;

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
