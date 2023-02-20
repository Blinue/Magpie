#pragma once
#include "AboutPage.g.h"

namespace winrt::Magpie::App::implementation {

struct AboutPage : AboutPageT<AboutPage> {
	Magpie::App::AboutViewModel ViewModel() const noexcept {
		return _viewModel;
	}

	void BugReportButton_Click(IInspectable const&, RoutedEventArgs const&);
	void FeatureRequestButton_Click(IInspectable const&, RoutedEventArgs const&);
	void DiscussionsButton_Click(IInspectable const&, RoutedEventArgs const&);

private:
	Magpie::App::AboutViewModel _viewModel;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct AboutPage : AboutPageT<AboutPage, implementation::AboutPage> {
};

}
