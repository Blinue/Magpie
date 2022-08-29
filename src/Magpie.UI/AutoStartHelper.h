#pragma once

namespace winrt::Magpie::UI {

struct AutoStartHelper {
	static bool EnableAutoStart(bool runElevated, const wchar_t* arguments);
	static bool DisableAutoStart();
	static bool IsAutoStartEnabled(std::wstring& arguments);
};

}
