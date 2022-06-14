#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif

using namespace winrt;

namespace winrt::Magpie::App::implementation
{
	HomePage::HomePage()
	{
		InitializeComponent();
	}
}
