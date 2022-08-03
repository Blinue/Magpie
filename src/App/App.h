#pragma once

#include "App.g.h"
#include "App.base.h"


namespace winrt::Magpie::App::implementation {

class App : public AppT2<App> {
public:
	App();
	~App();

	void SaveSettings();

	StartUpOptions Initialize(int);

	uint64_t HwndMain() const noexcept {
		return (uint64_t)_hwndMain;
	}

	void HwndMain(uint64_t value) noexcept;

	event_token HwndMainChanged(EventHandler<uint64_t> const& handler) {
		return _hwndMainChangedEvent.add(handler);
	}

	void HwndMainChanged(event_token const& token) noexcept {
		_hwndMainChangedEvent.remove(token);
	}

	Magpie::App::MainPage MainPage() const noexcept {
		return _mainPage;
	}

	void MainPage(Magpie::App::MainPage const& mainPage) noexcept;

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

	void RestartAsElevated() const noexcept;

private:
	HWND _hwndMain{};
	event<EventHandler<uint64_t>> _hwndMainChangedEvent;

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
