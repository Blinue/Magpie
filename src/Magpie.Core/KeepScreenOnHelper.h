#pragma once

namespace Magpie {

class KeepScreenOnHelper {
private:
	static void _DisableKeepScreenOn() noexcept;

public:
	using Guard = wil::unique_call<decltype(_DisableKeepScreenOn), _DisableKeepScreenOn, false>;

	static Guard EnableKeepScreenOn() noexcept;
};

}
