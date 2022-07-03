#pragma once

#include "App.g.h"
#include "App.base.h"
#include "Settings.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App::implementation {

class App : public AppT2<App> {
public:
	App();
	~App();

	void OnClose();

	bool Initialize(uint64_t hwndHost, uint64_t pWndRect, uint64_t pIsWndMaximized);

	uint64_t HwndHost() const noexcept {
		return _hwndHost;
	}

	Magpie::App::Settings Settings() const noexcept {
		return _settings;
	}

	Magpie::Runtime::MagRuntime MagRuntime() const noexcept {
		return _magRuntime;
	}

	event_token HostWndFocusChanged(EventHandler<bool> const& handler) {
		return _hostWndFocusChangedEvent.add(handler);
	}

	void HostWndFocusChanged(event_token const& token) noexcept {
		_hostWndFocusChangedEvent.remove(token);
	}

	void OnHostWndFocusChanged(bool isFocused);

	bool IsHostWndFocused() const noexcept {
		return _isHostWndFocused;
	}

	void OnHotkeyPressed(HotkeyAction action);

private:
	Magpie::App::Settings _settings;
	Magpie::Runtime::MagSettings _magSettings;
	Magpie::Runtime::MagRuntime _magRuntime;
	uint64_t _hwndHost{};

	event<EventHandler<bool>> _hostWndFocusChangedEvent;
	bool _isHostWndFocused = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

class App : public AppT<App, implementation::App> {
};

}
