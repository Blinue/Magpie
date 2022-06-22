#pragma once

#include "MagRuntime.g.h"

namespace winrt::Magpie::Runtime::implementation {

struct MagRuntime : MagRuntimeT<MagRuntime> {
	MagRuntime() = default;

	bool Scale(uint64_t hwndSrc, MagSettings const& settings);

	bool IsRunning() const;

private:
	std::thread _magWindThread;
};

}

namespace winrt::Magpie::Runtime::factory_implementation {

struct MagRuntime : MagRuntimeT<MagRuntime, implementation::MagRuntime> {
};

}
