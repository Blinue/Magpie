#pragma once

namespace winrt::Magpie::App {

struct AutoStartHelper {
	static bool EnableAutoStart(bool runElevated);
	static bool DisableAutoStart();
	static bool IsAutoStartEnabled();
};

}
