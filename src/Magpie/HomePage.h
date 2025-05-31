#pragma once
#include "HomePage.g.h"
#include "HomeViewModel.h"

namespace winrt::Magpie::implementation {

struct HomePage : HomePageT<HomePage> {
	void TimerSlider_Loaded(IInspectable const& sender, RoutedEventArgs const&) const;

	winrt::Magpie::HomeViewModel ViewModel() const noexcept {
		return *_viewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) const;

private:
	com_ptr<HomeViewModel> _viewModel = make_self<HomeViewModel>();
};

}

BASIC_FACTORY(HomePage)
