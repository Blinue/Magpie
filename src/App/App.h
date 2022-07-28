#pragma once

#include "App.g.h"
#include "App.base.h"


namespace winrt::Magpie::App::implementation {

class App : public AppT2<App> {
public:
	App();
	~App();

	void OnClose();

	void OnDestroy();

	bool Initialize(uint64_t hwndHost, uint64_t pWndRect, uint64_t pIsWndMaximized);

	uint64_t HwndHost() const noexcept {
		return _hwndHost;
	}

	Magpie::App::MainPage MainPage() const noexcept {
		return _mainPage;
	}

	void MainPage(Magpie::App::MainPage const& mainPage) noexcept {
		_mainPage = mainPage;
	}

	Windows::Graphics::Display::DisplayInformation DisplayInformation() const noexcept {
		return _displayInformation;
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
	uint64_t _hwndHost{};

	Magpie::App::MainPage _mainPage{ nullptr };
	Windows::Graphics::Display::DisplayInformation _displayInformation{ nullptr };

	event<EventHandler<bool>> _hostWndFocusChangedEvent;
	bool _isHostWndFocused = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

class App : public AppT<App, implementation::App> {
};

}
