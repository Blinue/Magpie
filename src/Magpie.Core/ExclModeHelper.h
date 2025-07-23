#pragma once

namespace Magpie {

struct ExclModeHelper {
	static wil::unique_mutex_nothrow EnterExclMode() noexcept;
};

}
