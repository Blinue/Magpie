#include "pch.h"
#include "AboutPage.h"
#if __has_include("AboutPage.g.cpp")
#include "AboutPage.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;


namespace winrt::Magpie::implementation {

AboutPage::AboutPage() {
	InitializeComponent();
}

}
