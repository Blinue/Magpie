#pragma once
#include "pch.h"
#include "MainPage.g.h"
#include "WinRTUtils.h"


namespace winrt::Magpie::App {
class ScalingProfile;
}

namespace winrt::Magpie::App::implementation {

struct MainPage : MainPageT<MainPage> {
	MainPage();

	void Loaded(IInspectable const&, RoutedEventArgs const&);

	void NavigationView_SelectionChanged(MUXC::NavigationView const&, MUXC::NavigationViewSelectionChangedEventArgs const& args);

	void NavigationView_PaneOpening(MUXC::NavigationView const&, IInspectable const&);

	void NavigationView_PaneClosing(MUXC::NavigationView const&, MUXC::NavigationViewPaneClosingEventArgs const&);

	void NavigationView_DisplayModeChanged(MUXC::NavigationView const&, MUXC::NavigationViewDisplayModeChangedEventArgs const&);

	void AddNavigationViewItem_Tapped(IInspectable const&, Input::TappedRoutedEventArgs const&);

	MUXC::NavigationView RootNavigationView();

	Magpie::App::NewProfileViewModel NewProfileViewModel() const noexcept {
		return _newProfileViewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const&, IInspectable const&);

	void NewProfileConfirmButton_Click(IInspectable const&, RoutedEventArgs const&);

private:
	void _UpdateTheme(bool updateIcons = true);

	fire_and_forget _LoadIcon(MUXC::NavigationViewItem const& item, const ScalingProfile& profile);

	IAsyncAction _UISettings_ColorValuesChanged(Windows::UI::ViewManagement::UISettings const&, IInspectable const&);

	void _UpdateUWPIcons();

	void _ScalingProfileService_ProfileAdded(ScalingProfile& profile);

	Windows::UI::ViewManagement::UISettings _uiSettings;
	Windows::UI::ViewManagement::UISettings::ColorValuesChanged_revoker _colorChangedRevoker;

	Magpie::App::NewProfileViewModel _newProfileViewModel;
	WinRTUtils::EventRevoker _profileAddedRevoker;

	Windows::Graphics::Display::DisplayInformation _displayInfomation{ nullptr };
};

}

namespace winrt::Magpie::App::factory_implementation {

struct MainPage : MainPageT<MainPage, implementation::MainPage> {
};

}
