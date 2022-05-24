#pragma once
#include "pch.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "MainPage.g.h"
#include "MicaBrush.h"


namespace winrt::Magpie::App::implementation
{
	struct MainPage : MainPageT<MainPage>
	{
		MainPage();

		~MainPage();

		void NavigationView_SelectionChanged(Microsoft::UI::Xaml::Controls::NavigationView const& sender, Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args);
		
		void NavigationView_DisplayModeChanged(Microsoft::UI::Xaml::Controls::NavigationView const& sender, Microsoft::UI::Xaml::Controls::NavigationViewDisplayModeChangedEventArgs const&);
		void NavigationView_PaneOpening(Microsoft::UI::Xaml::Controls::NavigationView const&, Windows::Foundation::IInspectable const&);
		void NavigationView_PaneClosing(Microsoft::UI::Xaml::Controls::NavigationView const&, Microsoft::UI::Xaml::Controls::NavigationViewPaneClosingEventArgs const&);

		void Theme(uint8_t theme);
		uint8_t Theme() const {
			return _theme;
		}

		void Initialize(uint64_t hwndHost);

		void OnHostFocusChanged(bool isFocused);

	private:
		void _UpdateTheme();

		Windows::Foundation::IAsyncAction _Settings_ColorValuesChanged(
			Windows::UI::ViewManagement::UISettings const&,
			Windows::Foundation::IInspectable const&
		);

		HWND _hwndHost = NULL;

		// 0: 浅色
		// 1: 深色
		// 2: 系统
		uint8_t _theme = 2;
		winrt::Windows::UI::ViewManagement::UISettings _uiSettings;
		winrt::event_token _colorChangedToken{};
		Magpie::App::MicaBrush _micaBrush{ nullptr };

		std::optional<bool> _isDarkTheme;
	};
}

namespace winrt::Magpie::App::factory_implementation
{
	struct MainPage : MainPageT<MainPage, implementation::MainPage>
	{
	};
}
