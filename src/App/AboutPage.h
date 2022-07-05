#pragma once
#include "AboutPage.g.h"


namespace winrt::Magpie::App::implementation {

struct AboutPage : AboutPageT<AboutPage> {
	AboutPage();
};

}

namespace winrt::Magpie::App::factory_implementation {

struct AboutPage : AboutPageT<AboutPage, implementation::AboutPage> {
};

}
