#pragma once
#include "AboutPage.g.h"


namespace winrt::Magpie::UI::implementation {

struct AboutPage : AboutPageT<AboutPage> {
	AboutPage();
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct AboutPage : AboutPageT<AboutPage, implementation::AboutPage> {
};

}
