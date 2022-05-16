#pragma once

#include "App.g.h"
#include "App.base.h"

namespace winrt::Magpie::App::implementation {
class App : public AppT2<App> {
public:
	App();
	~App();
};
}

namespace winrt::Magpie::App::factory_implementation {
class App : public AppT<App, implementation::App> {
};
}
