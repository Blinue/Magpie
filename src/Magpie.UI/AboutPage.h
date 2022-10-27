#pragma once
#include "AboutPage.g.h"


namespace winrt::Magpie::UI::implementation {

struct AboutPage : AboutPageT<AboutPage> {
	AboutPage();

	Magpie::UI::AboutViewModel ViewModel() const noexcept {
		return _viewModel;
	}

	void BugReportButton_Click(IInspectable const&, RoutedEventArgs const&);
	void FeatureRequestButton_Click(IInspectable const&, RoutedEventArgs const&);
	void DiscussionsButton_Click(IInspectable const&, RoutedEventArgs const&);

private:
	Magpie::UI::AboutViewModel _viewModel;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct AboutPage : AboutPageT<AboutPage, implementation::AboutPage> {
};

}
