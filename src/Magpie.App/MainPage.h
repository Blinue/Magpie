#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "MainPage.g.h"

namespace winrt::Magpie::App::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        void ThemeRadioButton_Checked(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::RoutedEventArgs const&);

        void HostWnd(uint64_t value);

        uint64_t HostWnd() const {
            return _hostWnd;
        }

    private:
        void _UpdateHostTheme();

        uint64_t _hostWnd{};

        // 0: 浅色
        // 1: 深色
        // 2: 系统
        uint32_t _theme = 2;
        winrt::Windows::UI::ViewManagement::UISettings _uiSettings;
        winrt::event_token _colorChangedToken{};
    };
}

namespace winrt::Magpie::App::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
