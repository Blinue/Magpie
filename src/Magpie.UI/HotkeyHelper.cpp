#include "pch.h"
#include "HotkeyHelper.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include <unordered_map>


namespace winrt::Magpie::UI {

std::string HotkeyHelper::ToString(winrt::Magpie::UI::HotkeyAction action) {
	using winrt::Magpie::UI::HotkeyAction;

	switch (action) {
	case HotkeyAction::Scale:
		return "Scale";
	case HotkeyAction::Overlay:
		return "Overlay";
	case HotkeyAction::COUNT_OR_NONE:
		return "None";
	default:
		break;
	}

	return {};
}

DWORD HotkeyHelper::StringToKeyCode(std::wstring_view str) {
	static std::unordered_map<std::wstring, DWORD, StrUtils::StringHash<wchar_t>, std::equal_to<>> map;
	if (map.empty()) {
		for (DWORD code : _GetValidKeyCodes()) {
			map[Win32Utils::GetKeyName(code)] = code;
		}
	}

	auto it = map.find(str);
	if (it == map.end()) {
		return 0;
	}

	return it->second;
}

const std::unordered_set<DWORD>& HotkeyHelper::_GetValidKeyCodes() {
	static std::unordered_set<DWORD> result;
	if (result.empty()) {
		// 字母
		for (DWORD i = 'A'; i <= 'Z'; ++i) {
			result.insert(i);
		}

		// 数字（顶部）
		for (DWORD i = '0'; i <= '9'; ++i) {
			result.insert(i);
		}

		// 数字（小键盘）
		for (DWORD i = VK_NUMPAD0; i <= VK_NUMPAD9; ++i) {
			result.insert(i);
		}

		// F1~F24
		for (DWORD i = VK_F1; i <= VK_F24; ++i) {
			result.insert(i);
		}

		// 空格、Page Up/Down、End、Home、方向键
		for (DWORD i = VK_SPACE; i <= VK_DOWN; ++i) {
			result.insert(i);
		}

		// 分号、等号、逗号、-、句号、/、`
		for (DWORD i = VK_OEM_1; i <= VK_OEM_3; ++i) {
			result.insert(i);
		}

		// [、\、]、'
		for (DWORD i = VK_OEM_4; i <= VK_OEM_7; ++i) {
			result.insert(i);
		}

		result.insert(VK_INSERT);	// Insert
		result.insert(VK_DELETE);	// Delete
		result.insert(VK_ADD);		// 加（小键盘）
		result.insert(VK_SUBTRACT);	// 减（小键盘）
		result.insert(VK_MULTIPLY);	// 乘（小键盘）
		result.insert(VK_DIVIDE);	// 除（小键盘）
		result.insert(VK_DECIMAL);	// .（小键盘）
		result.insert(VK_BACK);		// Backspace
		result.insert(VK_RETURN);	// 回车
	}

	return result;
}

} // namespace winrt::Magpie::UI

namespace winrt {

using Magpie::UI::HotkeyAction;

hstring to_hstring(HotkeyAction action) {
	switch (action) {
	case HotkeyAction::Scale:
		return L"Scale";
	case HotkeyAction::Overlay:
		return L"Overlay";
	case HotkeyAction::COUNT_OR_NONE:
		return L"None";
	default:
		break;
	}

	return {};
}

} // namespace winrt
