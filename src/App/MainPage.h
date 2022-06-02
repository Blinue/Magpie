#pragma once
#include "pch.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "MainPage.g.h"
#include "MicaBrush.h"
#include "Settings.h"


namespace winrt::Magpie::implementation
{
	struct MainPage : MainPageT<MainPage>
	{
		MainPage();

		~MainPage();

		void NavigationView_SelectionChanged(Microsoft::UI::Xaml::Controls::NavigationView const& sender, Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args);

		Windows::Foundation::IInspectable RootNavigationView();

		void Initialize(uint64_t hwndHost);

		void OnHostFocusChanged(bool isFocused);

	private:
		void _UpdateTheme();

		Windows::Foundation::IAsyncAction _Settings_ColorValuesChanged(
			Windows::UI::ViewManagement::UISettings const&,
			Windows::Foundation::IInspectable const&
		);

		HWND _hwndHost = NULL;

		Windows::UI::ViewManagement::UISettings _uiSettings;
		event_token _colorChangedToken{};
		Magpie::MicaBrush _micaBrush{ nullptr };

		Magpie::Settings _settings{ nullptr };
		std::optional<bool> _isDarkTheme;
	};
}

namespace winrt::Magpie::factory_implementation
{
	struct MainPage : MainPageT<MainPage, implementation::MainPage>
	{
	};
}
