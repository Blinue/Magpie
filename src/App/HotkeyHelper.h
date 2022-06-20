#pragma once
#include "pch.h"
#include <winrt/Magpie.App.h>
#include <unordered_set>


struct HotkeyHelper {
	static std::string ToString(winrt::Magpie::App::HotkeyAction action);

	static bool IsValidKeyCode(DWORD code) {
		return _GetValidKeyCodes().contains(code);
	}

	static DWORD StringToKeyCode(const std::wstring& str);

private:
	static const std::unordered_set<DWORD>& _GetValidKeyCodes();
};

namespace winrt {

// 将 HotkeyAction 映射为字符串
hstring to_hstring(Magpie::App::HotkeyAction action);

}
