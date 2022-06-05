#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif

using namespace winrt;

namespace winrt::Magpie::implementation
{
	HomePage::HomePage()
	{
		InitializeComponent();
	}
}
