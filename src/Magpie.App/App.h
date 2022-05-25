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

	bool Initialize(Magpie::App::Settings settings);

	Magpie::App::Settings Settings() const {
		return _settings;
	}

private:
	Magpie::App::Settings _settings;
};

}

namespace winrt::Magpie::App::factory_implementation {

class App : public AppT<App, implementation::App> {
};

}
