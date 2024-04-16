#pragma once
#include "Win32Utils.h"

namespace Magpie::Core {

struct ExclModeHelper {
	static wil::unique_mutex_nothrow EnterExclMode() noexcept;
};

}
