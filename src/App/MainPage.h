#pragma once
#include "pch.h"
#include "MainPage.g.h"


namespace winrt::Magpie::App::implementation
{
	struct MainPage : MainPageT<MainPage>
	{
		MainPage();

		~MainPage();

		void Loaded(IInspectable const&, RoutedEventArgs const&);

		void NavigationView_SelectionChanged(MUXC::NavigationView const&, MUXC::NavigationViewSelectionChangedEventArgs const& args);

		void NavigationView_PaneOpening(MUXC::NavigationView const&, IInspectable const&);

		void NavigationView_PaneClosing(MUXC::NavigationView const&, MUXC::NavigationViewPaneClosingEventArgs const&);

		void NavigationView_DisplayModeChanged(MUXC::NavigationView const&, MUXC::NavigationViewDisplayModeChangedEventArgs const&);

		IAsyncAction AddNavigationViewItem_Tapped(IInspectable const&, Input::TappedRoutedEventArgs const&);

		MUXC::NavigationView RootNavigationView();

	private:
		void _NewProfileDialog_Closing(Controls::ContentDialog const&, Controls::ContentDialogClosingEventArgs const& args);

		void _UpdateTheme();

		IAsyncAction _Settings_ColorValuesChanged(Windows::UI::ViewManagement::UISettings const&, IInspectable const&);

		Windows::UI::ViewManagement::UISettings _uiSettings;
		event_token _colorChangedToken{};

		Controls::ContentDialog _newProfileDialog{ nullptr };
	};
}

namespace winrt::Magpie::App::factory_implementation
{
	struct MainPage : MainPageT<MainPage, implementation::MainPage>
	{
	};
}
