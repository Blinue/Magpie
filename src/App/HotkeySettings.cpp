#include "pch.h"
#include "HotkeySettings.h"
#if __has_include("HotkeySettings.g.cpp")
#include "HotkeySettings.g.cpp"
#endif

#include "Win32Utils.h"


namespace winrt::Magpie::App::implementation {

void HotkeySettings::CopyFrom(const Magpie::App::HotkeySettings& other) {
	HotkeySettings* otherImpl = get_self<HotkeySettings>(other.as<default_interface<HotkeySettings>>());
	_win = otherImpl->_win;
	_ctrl = otherImpl->_ctrl;
	_alt = otherImpl->_alt;
	_shift = otherImpl->_shift;
	_code = otherImpl->_code;
}

bool HotkeySettings::IsEmpty() const {
	return !_win && !_ctrl && !_alt && !_shift && _code == 0;
}

bool HotkeySettings::Equals(const Magpie::App::HotkeySettings& other) const {
	HotkeySettings* otherImpl = get_self<HotkeySettings>(other.as<default_interface<HotkeySettings>>());
	return _win == otherImpl->_win
		&& _ctrl == otherImpl->_ctrl
		&& _alt == otherImpl->_alt
		&& _shift == otherImpl->_shift
		&& _code == otherImpl->_code;
}

IVector<IInspectable> HotkeySettings::GetKeyList() const {
	std::vector<IInspectable> shortcutList;
	if (_win) {
		shortcutList.push_back(box_value(VK_LWIN));
	}

	if (_ctrl) {
		shortcutList.push_back(box_value(Win32Utils::GetKeyName(VK_CONTROL)));
	}

	if (_alt) {
		shortcutList.push_back(box_value(Win32Utils::GetKeyName(VK_MENU)));
	}

	if (_shift) {
		shortcutList.push_back(box_value(Win32Utils::GetKeyName(VK_SHIFT)));
	}

	if (_code > 0) {
		switch (_code) {
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
		// case VK_BACK:
		// case VK_RETURN:
			shortcutList.push_back(box_value(_code));
			break;
		default:
			std::wstring localKey = Win32Utils::GetKeyName(_code);
			shortcutList.push_back(box_value(localKey));
			break;
		}
	}

	return single_threaded_vector(std::move(shortcutList));
}

bool HotkeySettings::Check() const {
	UINT modifiers = MOD_NOREPEAT;
	UINT modCount = 0;

	if (_win) {
		++modCount;
		modifiers |= MOD_WIN;
	}
	if (_ctrl) {
		++modCount;
		modifiers |= MOD_CONTROL;
	}
	if (_alt) {
		++modCount;
		modifiers |= MOD_ALT;
	}
	if (_shift) {
		++modCount;
		modifiers |= MOD_SHIFT;
	}

	if (modCount == 0 || (modCount == 1 && _code == 0)) {
		// 必须存在 Modifier
		// 如果只有一个 Modifier 则必须存在 Virtual Key
		return false;
	}

	// 检测快捷键是否被占用
	if (!RegisterHotKey(NULL, (int)HotkeyAction::COUNT_OR_NONE, modifiers, _code)) {
		return false;
	}

	UnregisterHotKey(NULL, (int)HotkeyAction::COUNT_OR_NONE);
	return true;
}

void HotkeySettings::Clear() {
	_win = false;
	_ctrl = false;
	_alt = false;
	_shift = false;
	_code = 0;
}

bool HotkeySettings::FromString(const hstring& str) {
	return false;
}

hstring HotkeySettings::ToString() const {
	std::wstring output;

	if (_win) {
		output.append(L"Win+");
	}

	if (_ctrl) {
		output.append(L"Ctrl+");
	}

	if (_alt) {
		output.append(L"Alt+");
	}

	if (_shift) {
		output.append(L"Shift+");
	}

	if (_code > 0) {
		output.append(Win32Utils::GetKeyName(_code));
	} else if (output.size() > 1) {
		output.pop_back();
	}

	return hstring(output);
}

}
