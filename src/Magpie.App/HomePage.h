#pragma once

#include "HomePage.g.h"

namespace winrt::Magpie::implementation
{
	struct HomePage : HomePageT<HomePage>
	{
		HomePage();

	};
}

namespace winrt::Magpie::factory_implementation
{
	struct HomePage : HomePageT<HomePage, implementation::HomePage>
	{
	};
}
