#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::Magpie::App::implementation
{
    HomePage::HomePage()
    {
        InitializeComponent();
    }

    int32_t HomePage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void HomePage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void HomePage::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        Button().Content(box_value(L"Clicked"));
    }
}
