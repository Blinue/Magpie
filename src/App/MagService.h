#pragma once
#include "MagService.g.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App::implementation {

struct MagService : MagServiceT<MagService> {
	MagService(Magpie::App::Settings const& settings, Magpie::Runtime::MagRuntime const& magRuntime, CoreDispatcher const& dispatcher);

	uint64_t WndToRestore() const noexcept {
		return _wndToRestore;
	}

	event_token WndToRestoreChanged(EventHandler<uint64_t> const& handler) {
		return _wndToRestoreChangedEvent.add(handler);
	}

	void WndToRestoreChanged(event_token const& token) noexcept {
		_wndToRestoreChangedEvent.remove(token);
	}

private:
	Magpie::App::Settings _settings{ nullptr };
	Magpie::Runtime::MagRuntime _magRuntime{ nullptr };

	Magpie::Runtime::MagRuntime::IsRunningChanged_revoker _isRunningChangedRevoker;

	HWND _curSrcWnd = 0;
	uint64_t _wndToRestore = 0;
	event<EventHandler<uint64_t>> _wndToRestoreChangedEvent;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct MagService : MagServiceT<MagService, implementation::MagService> {
};

}
