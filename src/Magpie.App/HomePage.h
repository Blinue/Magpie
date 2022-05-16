#pragma once

#include "HomePage.g.h"

namespace winrt::Magpie::App::implementation
{
    struct HomePage : HomePageT<HomePage>
    {
        HomePage();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::Magpie::App::factory_implementation
{
    struct HomePage : HomePageT<HomePage, implementation::HomePage>
    {
    };
}
