#pragma once
#include "App.g.h"
#include "App.base.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>

namespace winrt::Magpie::App::implementation {

class App : public AppT2<App> {
public:
	App();
	~App();

	void Close();

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

	// 在由外部源引发的回调中可能返回 nullptr
	// 这是因为用户关闭主窗口后 MainPage 不会立刻析构
	Magpie::App::MainPage MainPage() const noexcept {
		return _mainPage.get();
	}
	
	void MainPage(Magpie::App::MainPage const& mainPage) noexcept;

	void RestartAsElevated() const noexcept;

private:
	Hosting::WindowsXamlManager _windowsXamlManager{ nullptr };

	HWND _hwndMain{};
	event<EventHandler<uint64_t>> _hwndMainChangedEvent;

	weak_ref<Magpie::App::MainPage> _mainPage{ nullptr };

	event<EventHandler<bool>> _hostWndFocusChangedEvent;
	bool _isHostWndFocused = false;
	bool _isClosed = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

class App : public AppT<App, implementation::App> {
};

}
