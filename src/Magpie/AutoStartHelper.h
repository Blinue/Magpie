#pragma once

namespace Magpie {

struct AutoStartHelper {
	static bool EnableAutoStart(bool runElevated) noexcept;
	static bool DisableAutoStart() noexcept;
	static bool IsAutoStartEnabled() noexcept;
};

}
