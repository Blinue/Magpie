#include "pch.h"
#include "Hotkey.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include "HotkeyHelper.h"
#include "SmallVector.h"

namespace winrt::Magpie::App {

bool Hotkey::IsEmpty() const noexcept {
	return !win && !ctrl && !alt && !shift && code == 0;
}

SmallVector<std::variant<uint8_t, std::wstring>, 5> Hotkey::GetKeyList() const noexcept {
	SmallVector<std::variant<uint8_t, std::wstring>, 5> keyList;
	if (win) {
		keyList.emplace_back((uint8_t)VK_LWIN);
	}

	if (ctrl) {
		keyList.emplace_back(L"Ctrl");
	}

	if (alt) {
		keyList.emplace_back(L"Alt");
	}

	if (shift) {
		keyList.emplace_back(L"Shift");
	}

	if (code > 0) {
		switch (code) {
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
		// case VK_BACK:
		// case VK_RETURN:
			keyList.emplace_back(code);
			break;
		default:
			const std::wstring& localKey = Win32Utils::GetKeyName(code);
			keyList.emplace_back(localKey);
			break;
		}
	}

	return keyList;
}

HotkeyError Hotkey::Check() const noexcept {
	UINT modifiers = MOD_NOREPEAT;
	UINT modCount = 0;

	if (win) {
		++modCount;
		modifiers |= MOD_WIN;
	}
	if (ctrl) {
		++modCount;
		modifiers |= MOD_CONTROL;
	}
	if (alt) {
		++modCount;
		modifiers |= MOD_ALT;
	}
	if (shift) {
		++modCount;
		modifiers |= MOD_SHIFT;
	}

	if (modCount == 0 || (modCount == 1 && code == 0)) {
		// 必须存在 Modifier
		// 如果只有一个 Modifier 则必须存在 Virtual Key
		return HotkeyError::Invalid;
	}

	// 检测快捷键是否被占用
	if (!RegisterHotKey(NULL, (int)ShortcutAction::COUNT_OR_NONE, modifiers, code)) {
		return HotkeyError::Occupied;
	}

	UnregisterHotKey(NULL, (int)ShortcutAction::COUNT_OR_NONE);
	return HotkeyError::NoError;
}

void Hotkey::Clear() noexcept {
	win = false;
	ctrl = false;
	alt = false;
	shift = false;
	code = 0;
}

std::wstring Hotkey::ToString() const noexcept {
	std::wstring output;

	if (win) {
		output.append(L"Win+");
	}

	if (ctrl) {
		output.append(L"Ctrl+");
	}

	if (alt) {
		output.append(L"Alt+");
	}

	if (shift) {
		output.append(L"Shift+");
	}

	if (code > 0) {
		output.append(Win32Utils::GetKeyName(code));
	} else if (output.size() > 1) {
		output.pop_back();
	}

	return output;
}

}
