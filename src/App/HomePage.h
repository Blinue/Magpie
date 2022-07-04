#pragma once
#include "HomePage.g.h"
#include <winrt/Magpie.Runtime.h>
#include "WinRTUtils.h"


namespace winrt::Magpie::App::implementation {

struct HomePage : HomePageT<HomePage> {
	HomePage();

	Magpie::App::HomeViewModel ViewModel() const noexcept {
		return _viewModel;
	}

private:
	void _MagService_WndToRestoreChanged(uint64_t);

	void _UpdateAutoRestoreState();

	Magpie::App::HomeViewModel _viewModel;

	WinRTUtils::EventRevoker _wndToRestoreChangedRevoker;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HomePage : HomePageT<HomePage, implementation::HomePage> {
};

}
