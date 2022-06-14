#include "pch.h"
#include "AboutPage.h"
#if __has_include("AboutPage.g.cpp")
#include "AboutPage.g.cpp"
#endif

using namespace winrt;


namespace winrt::Magpie::App::implementation {

AboutPage::AboutPage() {
	InitializeComponent();
}

}
