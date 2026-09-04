#pragma once

namespace Magpie {

struct KeepScreenOnHelper {
	static void DisableKeepScreenOn() noexcept;

	using unique_cancel = wil::unique_call<decltype(DisableKeepScreenOn), DisableKeepScreenOn, false>;

	static unique_cancel EnableKeepScreenOn() noexcept;
};

}
