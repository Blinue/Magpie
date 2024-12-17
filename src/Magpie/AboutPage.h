#pragma once
#include "AboutPage.g.h"
#include "AboutViewModel.h"

namespace winrt::Magpie::implementation {

struct AboutPage : AboutPageT<AboutPage> {
	winrt::Magpie::AboutViewModel ViewModel() const noexcept {
		return *_viewModel;
	}

	void VersionTextBlock_DoubleTapped(IInspectable const&, Input::DoubleTappedRoutedEventArgs const&);

	void BugReportButton_Click(IInspectable const&, RoutedEventArgs const&);
	void FeatureRequestButton_Click(IInspectable const&, RoutedEventArgs const&);
	void DiscussionsButton_Click(IInspectable const&, RoutedEventArgs const&);

private:
	winrt::com_ptr<AboutViewModel> _viewModel = make_self<AboutViewModel>();
};

}

namespace winrt::Magpie::factory_implementation {

struct AboutPage : AboutPageT<AboutPage, implementation::AboutPage> {
};

}
