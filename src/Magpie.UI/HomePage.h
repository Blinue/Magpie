#pragma once
#include "HomePage.g.h"

namespace winrt::Magpie::UI::implementation {

struct HomePage : HomePageT<HomePage> {
	HomePage();

	Magpie::UI::HomeViewModel ViewModel() const noexcept {
		return _viewModel;
	}

private:
	Magpie::UI::HomeViewModel _viewModel;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct HomePage : HomePageT<HomePage, implementation::HomePage> {
};

}
