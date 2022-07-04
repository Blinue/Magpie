#pragma once
#include "HomePage.g.h"


namespace winrt::Magpie::App::implementation {

struct HomePage : HomePageT<HomePage> {
	HomePage();

	Magpie::App::HomeViewModel ViewModel() const noexcept {
		return _viewModel;
	}

private:
	Magpie::App::HomeViewModel _viewModel;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HomePage : HomePageT<HomePage, implementation::HomePage> {
};

}
