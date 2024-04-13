#pragma once

namespace winrt::Magpie::App {

struct AutoStartHelper {
	static bool EnableAutoStart(bool runElevated, const wchar_t* arguments) noexcept;
	static bool DisableAutoStart() noexcept;
	static bool IsAutoStartEnabled(std::wstring& arguments) noexcept;
};

}
