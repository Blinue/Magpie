#pragma once
#include "MainPage.g.h"
#include "WinRTUtils.h"

namespace winrt::Magpie::App {
struct Profile;
}

namespace winrt::Magpie::App::implementation {

struct MainPage : MainPageT<MainPage> {
	MainPage();

	~MainPage();

	void Loaded(IInspectable const&, RoutedEventArgs const&);

	void NavigationView_SelectionChanged(MUXC::NavigationView const&, MUXC::NavigationViewSelectionChangedEventArgs const& args);

	void NavigationView_PaneOpening(MUXC::NavigationView const&, IInspectable const&);

	void NavigationView_PaneClosing(MUXC::NavigationView const&, MUXC::NavigationViewPaneClosingEventArgs const&);

	void NavigationView_DisplayModeChanged(MUXC::NavigationView const&, MUXC::NavigationViewDisplayModeChangedEventArgs const&);

	fire_and_forget NavigationView_ItemInvoked(MUXC::NavigationView const&, MUXC::NavigationViewItemInvokedEventArgs const& args);

	Magpie::App::NewProfileViewModel NewProfileViewModel() const noexcept {
		return _newProfileViewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const&, IInspectable const&);

	void NewProfileConfirmButton_Click(IInspectable const&, RoutedEventArgs const&);

	void NewProfileNameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args);

	void NavigateToAboutPage();

private:
	void _UpdateTheme(bool updateIcons = true);

	fire_and_forget _LoadIcon(MUXC::NavigationViewItem const& item, const Profile& profile);

	fire_and_forget _UISettings_ColorValuesChanged(Windows::UI::ViewManagement::UISettings const&, IInspectable const&);

	void _UpdateIcons(bool skipDesktop);

	void _ProfileService_ProfileAdded(Profile& profile);

	void _ProfileService_ProfileRenamed(uint32_t idx);

	void _ProfileService_ProfileRemoved(uint32_t idx);

	void _ProfileService_ProfileReordered(uint32_t profileIdx, bool isMoveUp);

	WinRTUtils::EventRevoker _themeChangedRevoker;

	Windows::UI::ViewManagement::UISettings _uiSettings;
	Windows::UI::ViewManagement::UISettings::ColorValuesChanged_revoker _colorValuesChangedRevoker;

	Magpie::App::NewProfileViewModel _newProfileViewModel;
	WinRTUtils::EventRevoker _profileAddedRevoker;
	WinRTUtils::EventRevoker _profileRenamedRevoker;
	WinRTUtils::EventRevoker _profileRemovedRevoker;
	WinRTUtils::EventRevoker _profileMovedRevoker;

	Windows::Graphics::Display::DisplayInformation _displayInformation{ nullptr };
	Windows::Graphics::Display::DisplayInformation::DpiChanged_revoker _dpiChangedRevoker;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct MainPage : MainPageT<MainPage, implementation::MainPage> {
};

}
