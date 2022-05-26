#include "pch.h"
#include "AboutPage.h"
#if __has_include("AboutPage.g.cpp")
#include "AboutPage.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::Magpie::App::implementation
{
	AboutPage::AboutPage()
	{
		InitializeComponent();
	}

	int32_t AboutPage::MyProperty()
	{
		throw hresult_not_implemented();
	}

	void AboutPage::MyProperty(int32_t /* value */)
	{
		throw hresult_not_implemented();
	}

	void AboutPage::ClickHandler(IInspectable const&, RoutedEventArgs const&)
	{
		Button().Content(box_value(L"Clicked"));
	}
}
