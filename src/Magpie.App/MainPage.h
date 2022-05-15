#pragma once
#include "pch.h"
#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "MainPage.g.h"
#include "MicaBrush.h"


namespace winrt::Magpie::App::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        ~MainPage();

        void ThemeRadioButton_Checked(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::RoutedEventArgs const&);

        void HostWnd(uint64_t value);

        uint64_t HostWnd() const {
            return _hostWnd;
        }

        void OnHostFocusChanged(bool isFocused);

    private:
        void _UpdateHostTheme();

        uint64_t _hostWnd{};

        // 0: 浅色
        // 1: 深色
        // 2: 系统
        uint32_t _theme = 2;
        winrt::Windows::UI::ViewManagement::UISettings _uiSettings;
        winrt::event_token _colorChangedToken{};
        Magpie::App::MicaBrush _micaBrush{ nullptr };
    };
}

namespace winrt::Magpie::App::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
