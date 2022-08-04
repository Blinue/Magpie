#pragma once

namespace winrt::Magpie::App {

struct AutoStartHelper {
	static bool CreateAutoStartTask(bool runElevated);

	static bool DeleteAutoStartTask();

	static bool IsAutoStartTaskActive();
};

}
