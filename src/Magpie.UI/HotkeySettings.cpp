#include "pch.h"
#include "HotkeySettings.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include "HotkeyHelper.h"


namespace winrt::Magpie::UI {

bool HotkeySettings::IsEmpty() const noexcept {
	return !win && !ctrl && !alt && !shift && code == 0;
}

std::vector<std::variant<uint32_t, std::wstring>> HotkeySettings::GetKeyList() const noexcept {
	std::vector<std::variant<uint32_t, std::wstring>> shortcutList;
	if (win) {
		shortcutList.emplace_back((uint32_t)VK_LWIN);
	}

	if (ctrl) {
		shortcutList.emplace_back(L"Ctrl");
	}

	if (alt) {
		shortcutList.emplace_back(L"Alt");
	}

	if (shift) {
		shortcutList.emplace_back(L"Shift");
	}

	if (code > 0) {
		switch (code) {
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
		// case VK_BACK:
		// case VK_RETURN:
			shortcutList.emplace_back(code);
			break;
		default:
			std::wstring localKey = Win32Utils::GetKeyName(code);
			shortcutList.emplace_back(localKey);
			break;
		}
	}

	return shortcutList;
}

HotkeyError HotkeySettings::Check() const noexcept {
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
	if (!RegisterHotKey(NULL, (int)HotkeyAction::COUNT_OR_NONE, modifiers, code)) {
		return HotkeyError::Occupied;
	}

	UnregisterHotKey(NULL, (int)HotkeyAction::COUNT_OR_NONE);
	return HotkeyError::NoError;
}

void HotkeySettings::Clear() noexcept {
	win = false;
	ctrl = false;
	alt = false;
	shift = false;
	code = 0;
}

bool HotkeySettings::FromString(std::wstring_view str) noexcept {
	bool winT = false;
	bool ctrlT = false;
	bool altT = false;
	bool shiftT = false;
	uint32_t codeT = 0;

	if (!str.empty()) {
		std::vector<std::wstring_view> parts = StrUtils::Split(str, L'+');
		for (std::wstring_view& part : parts) {
			StrUtils::Trim(part);

			if (part.empty()) {
				return false;
			}

			if (part == L"Win") {
				if (winT) {
					return false;
				}

				winT = true;
			} else if (part == L"Ctrl") {
				if (ctrlT) {
					return false;
				}

				ctrlT = true;
			} else if (part == L"Alt") {
				if (altT) {
					return false;
				}

				altT = true;
			} else if (part == L"Shift") {
				if (shiftT) {
					return false;
				}

				shiftT = true;
			} else {
				if (codeT) {
					return false;
				}

				codeT = HotkeyHelper::StringToKeyCode(part);
				if (codeT <= 0) {
					return false;
				}
			}
		}
	}

	win = winT;
	ctrl = ctrlT;
	alt = altT;
	shift = shiftT;
	code = codeT;
	return true;
}

std::wstring HotkeySettings::ToString() const noexcept {
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
