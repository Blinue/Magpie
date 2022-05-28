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

	bool Initialize(Magpie::Settings settings);

	Magpie::Settings Settings() const {
		return _settings;
	}

private:
	Magpie::Settings _settings;
};

}

namespace winrt::Magpie::factory_implementation {

class App : public AppT<App, implementation::App> {
};

}
