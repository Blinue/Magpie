#pragma once
#include "Win32Utils.h"

namespace Magpie::Core {

struct ExclModeHelper {
	static Win32Utils::ScopedHandle EnterExclMode() noexcept;
	static void ExitExclMode(Win32Utils::ScopedHandle& mutex) noexcept;
};

}
