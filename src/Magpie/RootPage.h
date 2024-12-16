#pragma once
#include "RootPage.g.h"
#include "Event.h"
#include "AppSettings.h"

namespace Magpie {
struct Profile;
}

namespace winrt::Magpie::implementation {

struct RootPage : RootPageT<RootPage> {
	RootPage();
	~RootPage();

	void InitializeComponent();

	void Loaded(IInspectable const&, RoutedEventArgs const&);

	void NavigationView_SelectionChanged(MUXC::NavigationView const&, MUXC::NavigationViewSelectionChangedEventArgs const& args);

	void NavigationView_PaneOpening(MUXC::NavigationView const&, IInspectable const&);

	void NavigationView_PaneClosing(MUXC::NavigationView const&, MUXC::NavigationViewPaneClosingEventArgs const&);

	void NavigationView_DisplayModeChanged(MUXC::NavigationView const& nv, MUXC::NavigationViewDisplayModeChangedEventArgs const&);

	fire_and_forget NavigationView_ItemInvoked(MUXC::NavigationView const&, MUXC::NavigationViewItemInvokedEventArgs const& args);

	Magpie::NewProfileViewModel NewProfileViewModel() const noexcept {
		return _newProfileViewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const&, IInspectable const&) const;

	void NewProfileConfirmButton_Click(IInspectable const&, RoutedEventArgs const&);

	void NewProfileNameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args);

	void NavigateToAboutPage();

private:
	void _UpdateTheme(bool updateIcons);

	fire_and_forget _LoadIcon(MUXC::NavigationViewItem const& item, const ::Magpie::Profile& profile);

	void _UpdateColorValuesChangedRevoker();

	void _UISettings_ColorValuesChanged(Windows::UI::ViewManagement::UISettings const&, IInspectable const&);

	void _AppSettings_ThemeChanged(::Magpie::AppTheme);

	void _UpdateIcons(bool skipDesktop);

	void _ProfileService_ProfileAdded(::Magpie::Profile& profile);

	void _ProfileService_ProfileRenamed(uint32_t idx);

	void _ProfileService_ProfileRemoved(uint32_t idx);

	void _ProfileService_ProfileReordered(uint32_t profileIdx, bool isMoveUp);

	::Magpie::Core::EventRevoker _themeChangedRevoker;

	Windows::UI::ViewManagement::UISettings _uiSettings;
	Windows::UI::ViewManagement::UISettings::ColorValuesChanged_revoker _colorValuesChangedRevoker;

	Magpie::NewProfileViewModel _newProfileViewModel;
	::Magpie::Core::EventRevoker _profileAddedRevoker;
	::Magpie::Core::EventRevoker _profileRenamedRevoker;
	::Magpie::Core::EventRevoker _profileRemovedRevoker;
	::Magpie::Core::EventRevoker _profileMovedRevoker;

	Windows::Graphics::Display::DisplayInformation _displayInformation{ nullptr };
	Windows::Graphics::Display::DisplayInformation::DpiChanged_revoker _dpiChangedRevoker;
};

}

namespace winrt::Magpie::factory_implementation {

struct RootPage : RootPageT<RootPage, implementation::RootPage> {
};

}
