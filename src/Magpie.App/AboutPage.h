#pragma once

#include "AboutPage.g.h"

namespace winrt::Magpie::App::implementation
{
	struct AboutPage : AboutPageT<AboutPage>
	{
		AboutPage();

		int32_t MyProperty();
		void MyProperty(int32_t value);

		void ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
	};
}

namespace winrt::Magpie::App::factory_implementation
{
	struct AboutPage : AboutPageT<AboutPage, implementation::AboutPage>
	{
	};
}
