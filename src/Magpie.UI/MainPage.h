#pragma once
#include "pch.h"
#include "MainPage.g.h"
#include "WinRTUtils.h"


namespace winrt::Magpie::UI {
struct ScalingProfile;
}

namespace winrt::Magpie::UI::implementation {

struct MainPage : MainPageT<MainPage> {
	MainPage();

	~MainPage();

	void Loaded(IInspectable const&, RoutedEventArgs const&);

	void NavigationView_SelectionChanged(MUXC::NavigationView const&, MUXC::NavigationViewSelectionChangedEventArgs const& args);

	void NavigationView_PaneOpening(MUXC::NavigationView const&, IInspectable const&);

	void NavigationView_PaneClosing(MUXC::NavigationView const&, MUXC::NavigationViewPaneClosingEventArgs const&);

	void NavigationView_DisplayModeChanged(MUXC::NavigationView const&, MUXC::NavigationViewDisplayModeChangedEventArgs const&);

	IAsyncAction NavigationView_ItemInvoked(MUXC::NavigationView const&, MUXC::NavigationViewItemInvokedEventArgs const& args);

	MUXC::NavigationView RootNavigationView();

	Magpie::UI::NewProfileViewModel NewProfileViewModel() const noexcept {
		return _newProfileViewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const&, IInspectable const&);

	void NewProfileConfirmButton_Click(IInspectable const&, RoutedEventArgs const&);

private:
	void _UpdateTheme(bool updateIcons = true);

	fire_and_forget _LoadIcon(MUXC::NavigationViewItem const& item, const ScalingProfile& profile);

	IAsyncAction _UISettings_ColorValuesChanged(Windows::UI::ViewManagement::UISettings const&, IInspectable const&);

	void _UpdateIcons(bool skipDesktop);

	void _ScalingProfileService_ProfileAdded(ScalingProfile& profile);

	void _ScalingProfileService_ProfileRenamed(uint32_t idx);

	void _ScalingProfileService_ProfileRemoved(uint32_t idx);

	void _ScalingProfileService_ProfileReordered(uint32_t profileIdx, bool isMoveUp);

	WinRTUtils::EventRevoker _themeChangedRevoker;

	Windows::UI::ViewManagement::UISettings _uiSettings;
	Windows::UI::ViewManagement::UISettings::ColorValuesChanged_revoker _colorValuesChangedRevoker;

	Magpie::UI::NewProfileViewModel _newProfileViewModel;
	WinRTUtils::EventRevoker _profileAddedRevoker;
	WinRTUtils::EventRevoker _profileRenamedRevoker;
	WinRTUtils::EventRevoker _profileRemovedRevoker;
	WinRTUtils::EventRevoker _profileReorderdRevoker;

	Windows::Graphics::Display::DisplayInformation _displayInformation{ nullptr };
	Windows::Graphics::Display::DisplayInformation::DpiChanged_revoker _dpiChangedRevoker;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct MainPage : MainPageT<MainPage, implementation::MainPage> {
};

}
