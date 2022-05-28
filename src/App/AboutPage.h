#pragma once

#include "AboutPage.g.h"

namespace winrt::Magpie::implementation
{
	struct AboutPage : AboutPageT<AboutPage>
	{
		AboutPage();
	};
}

namespace winrt::Magpie::factory_implementation
{
	struct AboutPage : AboutPageT<AboutPage, implementation::AboutPage>
	{
	};
}
