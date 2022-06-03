#pragma once

#include "App.g.h"
#include "App.base.h"
#include "Settings.h"

namespace winrt::Magpie::implementation {

class App : public AppT2<App> {
public:
	App();
	~App();

	void OnClose();

	bool Initialize(Magpie::Settings settings, uint64_t hwndHost);

	uint64_t HwndHost() const {
		return _hwndHost;
	}

	Magpie::Settings Settings() const {
		return _settings;
	}

private:
	Magpie::Settings _settings;
	uint64_t _hwndHost;
};

}

namespace winrt::Magpie::factory_implementation {

class App : public AppT<App, implementation::App> {
};

}
