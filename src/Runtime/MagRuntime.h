#pragma once

#include "MagRuntime.g.h"

namespace winrt::Magpie::Runtime::implementation {

struct MagRuntime : MagRuntimeT<MagRuntime> {
	MagRuntime() = default;

	void Run(uint64_t hwndSrc, MagSettings const& settings);

	void ToggleOverlay();

	void Stop();

	bool IsRunning() const {
		return _running;
	}

private:
	std::thread _magWindThread;
	std::atomic<bool> _running = false;
	DispatcherQueueController _dqc{ nullptr };
};

}

namespace winrt::Magpie::Runtime::factory_implementation {

struct MagRuntime : MagRuntimeT<MagRuntime, implementation::MagRuntime> {
};

}
