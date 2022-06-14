#pragma once

#include "App.g.h"
#include "App.base.h"
#include "Settings.h"

namespace winrt::Magpie::App::implementation {

class App : public AppT2<App> {
public:
	App();
	~App();

	void OnClose();

	bool Initialize(Magpie::App::Settings settings, uint64_t hwndHost);

	uint64_t HwndHost() const {
		return _hwndHost;
	}

	Magpie::App::Settings Settings() const {
		return _settings;
	}

	event_token HostWndFocusChanged(EventHandler<bool> const& handler);
	void HostWndFocusChanged(event_token const& token) noexcept;

	void OnHostWndFocusChanged(bool isFocused);
	bool IsHostWndFocused() const noexcept {
		return _isHostWndFocused;
	}

private:
	Magpie::App::Settings _settings{ nullptr };
	uint64_t _hwndHost{};

	event<EventHandler<bool>> _hostWndFocusChangedEvent;
	bool _isHostWndFocused = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

class App : public AppT<App, implementation::App> {
};

}
