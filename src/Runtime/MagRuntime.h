#pragma once

#include "MagRuntime.g.h"

namespace winrt::Magpie::Runtime::implementation {

struct MagRuntime : MagRuntimeT<MagRuntime> {
	MagRuntime() = default;

	~MagRuntime();

	void Run(uint64_t hwndSrc, MagSettings const& settings);

	void ToggleOverlay();

	void Stop();

	bool IsRunning() const {
		return _running;
	}

	uint64_t HwndSrc() const {
		return _running ? _hwndSrc : 0;
	}

	event_token IsRunningChanged(EventHandler<bool> const& handler);
	void IsRunningChanged(event_token const& token) noexcept;

private:
	std::thread _magWindThread;
	std::atomic<bool> _running = false;
	uint64_t _hwndSrc = 0;
	DispatcherQueueController _dqc{ nullptr };

	event<EventHandler<bool>> _isRunningChangedEvent;
};

}

namespace winrt::Magpie::Runtime::factory_implementation {

struct MagRuntime : MagRuntimeT<MagRuntime, implementation::MagRuntime> {
};

}
