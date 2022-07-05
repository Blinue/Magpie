#pragma once

#include "ScalingProfilePage.g.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App::implementation {

struct ScalingProfilePage : ScalingProfilePageT<ScalingProfilePage> {
	ScalingProfilePage();

	void OnNavigatedTo(Navigation::NavigationEventArgs const& args);

	Magpie::App::ScalingProfileViewModel ViewModel() const noexcept {
		return _viewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

private:
	Magpie::App::ScalingProfileViewModel _viewModel{ nullptr };
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ScalingProfilePage : ScalingProfilePageT<ScalingProfilePage, implementation::ScalingProfilePage> {
};

}
