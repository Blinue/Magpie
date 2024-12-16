#pragma once
#include "Win32Helper.h"

namespace Magpie {

struct ExclModeHelper {
	static wil::unique_mutex_nothrow EnterExclMode() noexcept;
};

}
