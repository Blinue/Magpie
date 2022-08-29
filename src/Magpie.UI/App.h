#pragma once

#include "App.g.h"
#include "App.base.h"


namespace winrt::Magpie::UI::implementation {

class App : public AppT2<App> {
public:
	App();
	~App();

	void SaveSettings();

	StartUpOptions Initialize(int);

	bool IsShowTrayIcon() const noexcept;

	event_token IsShowTrayIconChanged(EventHandler<bool> const& handler);

	void IsShowTrayIconChanged(event_token const& token);

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

	Magpie::UI::MainPage MainPage() const noexcept {
		return _mainPage;
	}

	void MainPage(Magpie::UI::MainPage const& mainPage) noexcept;

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

	void RestartAsElevated() const noexcept;

private:
	HWND _hwndMain{};
	event<EventHandler<uint64_t>> _hwndMainChangedEvent;

	Magpie::UI::MainPage _mainPage{ nullptr };
	Windows::Graphics::Display::DisplayInformation _displayInformation{ nullptr };

	event<EventHandler<bool>> _hostWndFocusChangedEvent;
	bool _isHostWndFocused = false;
};

}

namespace winrt::Magpie::UI::factory_implementation {

class App : public AppT<App, implementation::App> {
};

}
