#pragma once
#include "HomePage.g.h"

namespace winrt::Magpie::implementation {

struct HomePage : HomePageT<HomePage> {
	void TimerSlider_Loaded(IInspectable const& sender, RoutedEventArgs const&) const;

	Magpie::HomeViewModel ViewModel() const noexcept {
		return _viewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) const;

	void SimulateExclusiveFullscreenToggleSwitch_Toggled(IInspectable const& sender, RoutedEventArgs const&);

private:
	Magpie::HomeViewModel _viewModel;
};

}

namespace winrt::Magpie::factory_implementation {

struct HomePage : HomePageT<HomePage, implementation::HomePage> {
};

}
