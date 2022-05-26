#pragma once

#include "HomePage.g.h"

namespace winrt::Magpie::App::implementation
{
	struct HomePage : HomePageT<HomePage>
	{
		HomePage();

	};
}

namespace winrt::Magpie::App::factory_implementation
{
	struct HomePage : HomePageT<HomePage, implementation::HomePage>
	{
	};
}
