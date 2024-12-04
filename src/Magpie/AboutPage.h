#pragma once
#include "AboutPage.g.h"

namespace winrt::Magpie::implementation {

struct AboutPage : AboutPageT<AboutPage> {
	Magpie::AboutViewModel ViewModel() const noexcept {
		return _viewModel;
	}

	void VersionTextBlock_DoubleTapped(IInspectable const&, Input::DoubleTappedRoutedEventArgs const&);

	void BugReportButton_Click(IInspectable const&, RoutedEventArgs const&);
	void FeatureRequestButton_Click(IInspectable const&, RoutedEventArgs const&);
	void DiscussionsButton_Click(IInspectable const&, RoutedEventArgs const&);

private:
	Magpie::AboutViewModel _viewModel;
};

}

namespace winrt::Magpie::factory_implementation {

struct AboutPage : AboutPageT<AboutPage, implementation::AboutPage> {
};

}
