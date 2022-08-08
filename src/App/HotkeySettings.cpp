#include "pch.h"
#include "HotkeySettings.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include "HotkeyHelper.h"


namespace winrt::Magpie::App {

bool HotkeySettings::IsEmpty() const noexcept {
	return !Win && !Ctrl && !Alt && !Shift && Code == 0;
}

std::vector<std::variant<uint32_t, std::wstring>> HotkeySettings::GetKeyList() const noexcept {
	std::vector<std::variant<uint32_t, std::wstring>> shortcutList;
	if (Win) {
		shortcutList.emplace_back((uint32_t)VK_LWIN);
	}

	if (Ctrl) {
		shortcutList.emplace_back(L"Ctrl");
	}

	if (Alt) {
		shortcutList.emplace_back(L"Alt");
	}

	if (Shift) {
		shortcutList.emplace_back(L"Shift");
	}

	if (Code > 0) {
		switch (Code) {
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
		// case VK_BACK:
		// case VK_RETURN:
			shortcutList.emplace_back(Code);
			break;
		default:
			std::wstring localKey = Win32Utils::GetKeyName(Code);
			shortcutList.emplace_back(localKey);
			break;
		}
	}

	return shortcutList;
}

HotkeyError HotkeySettings::Check() const noexcept {
	UINT modifiers = MOD_NOREPEAT;
	UINT modCount = 0;

	if (Win) {
		++modCount;
		modifiers |= MOD_WIN;
	}
	if (Ctrl) {
		++modCount;
		modifiers |= MOD_CONTROL;
	}
	if (Alt) {
		++modCount;
		modifiers |= MOD_ALT;
	}
	if (Shift) {
		++modCount;
		modifiers |= MOD_SHIFT;
	}

	if (modCount == 0 || (modCount == 1 && Code == 0)) {
		// 必须存在 Modifier
		// 如果只有一个 Modifier 则必须存在 Virtual Key
		return HotkeyError::Invalid;
	}

	// 检测快捷键是否被占用
	if (!RegisterHotKey(NULL, (int)HotkeyAction::COUNT_OR_NONE, modifiers, Code)) {
		return HotkeyError::Occupied;
	}

	UnregisterHotKey(NULL, (int)HotkeyAction::COUNT_OR_NONE);
	return HotkeyError::NoError;
}

void HotkeySettings::Clear() noexcept {
	Win = false;
	Ctrl = false;
	Alt = false;
	Shift = false;
	Code = 0;
}

bool HotkeySettings::FromString(std::wstring_view str) noexcept {
	bool win = false;
	bool ctrl = false;
	bool alt = false;
	bool shift = false;
	uint32_t code = 0;

	if (!str.empty()) {
		std::vector<std::wstring_view> parts = StrUtils::Split(str, L'+');
		for (std::wstring_view& part : parts) {
			StrUtils::Trim(part);

			if (part.empty()) {
				return false;
			}

			if (part == L"Win") {
				if (win) {
					return false;
				}

				win = true;
			} else if (part == L"Ctrl") {
				if (ctrl) {
					return false;
				}

				ctrl = true;
			} else if (part == L"Alt") {
				if (alt) {
					return false;
				}

				alt = true;
			} else if (part == L"Shift") {
				if (shift) {
					return false;
				}

				shift = true;
			} else {
				if (code) {
					return false;
				}

				code = HotkeyHelper::StringToKeyCode(part);
				if (code <= 0) {
					return false;
				}
			}
		}
	}

	Win = win;
	Ctrl = ctrl;
	Alt = alt;
	Shift = shift;
	Code = code;
	return true;
}

std::wstring HotkeySettings::ToString() const noexcept {
	std::wstring output;

	if (Win) {
		output.append(L"Win+");
	}

	if (Ctrl) {
		output.append(L"Ctrl+");
	}

	if (Alt) {
		output.append(L"Alt+");
	}

	if (Shift) {
		output.append(L"Shift+");
	}

	if (Code > 0) {
		output.append(Win32Utils::GetKeyName(Code));
	} else if (output.size() > 1) {
		output.pop_back();
	}

	return output;
}

}
