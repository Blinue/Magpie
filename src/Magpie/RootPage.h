#pragma once
#include "RootPage.g.h"
#include "DeleteConfirmationFlyoutContent.h"
#include "Event.h"
#include "NewProfileViewModel.h"

namespace Magpie {
struct Profile;
}

namespace winrt::Magpie::implementation {

struct TitleBarControl;

struct RootPage : RootPageT<RootPage> {
	~RootPage();

	void InitializeComponent();

	void RootPage_Loaded(IInspectable const&, RoutedEventArgs const&);

	void NavigationViewItemMenuFlyout_Opening(IInspectable const&, IInspectable const&);

	void LaunchMenuFlyoutItem_Click(IInspectable const& sender, RoutedEventArgs const& e);

	void OpenProgramLocationFlyoutItem_Click(IInspectable const& sender, RoutedEventArgs const& e);

	void DeleteProfileFlyoutItem_Click(IInspectable const& sender, RoutedEventArgs const& e);

	void DeleteFlyout_Opening(IInspectable const& sender, IInspectable const& e);

	void DeleteConfirmationButton_Click(IInspectable const&, RoutedEventArgs const&);

	void NavigationView_SelectionChanged(MUXC::NavigationView const&, MUXC::NavigationViewSelectionChangedEventArgs const& args);

	void NavigationView_PaneOpening(MUXC::NavigationView const&, IInspectable const&);

	void NavigationView_PaneClosing(MUXC::NavigationView const&, MUXC::NavigationViewPaneClosingEventArgs const&);

	void NavigationView_DisplayModeChanged(MUXC::NavigationView const& nv, MUXC::NavigationViewDisplayModeChangedEventArgs const&);

	void NavigationView_ItemInvoked(MUXC::NavigationView const&, MUXC::NavigationViewItemInvokedEventArgs const& args);

	winrt::Magpie::NewProfileViewModel NewProfileViewModel() const noexcept {
		return *_newProfileViewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const&, IInspectable const&) const;

	void NewProfileConfirmButton_Click(IInspectable const&, RoutedEventArgs const&);

	void NewProfileNameContextFlyout_Opening(IInspectable const&, IInspectable const&);

	void NewProfileNameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args);

	void NavigateToAboutPage();

	TitleBarControl& TitleBar();

private:
	void _UpdateTheme(bool updateIcons);

	fire_and_forget _LoadIcon(MUXC::NavigationViewItem const& item, const ::Magpie::Profile& profile);

	void _UpdateIcons(bool skipDesktop);

	void _ProfileService_ProfileAdded(::Magpie::Profile& profile);

	void _ProfileService_ProfileRenamed(uint32_t idx);

	void _ProfileService_ProfileRemoved(uint32_t idx);

	void _ProfileService_ProfileReordered(uint32_t profileIdx, bool isMoveUp);

	void _UpdateNewProfileNameTextBox(bool fillWithTitle);

	MUXC::NavigationViewItem _CreateProfileNavigationViewItem(const ::Magpie::Profile& profile);

	::Magpie::MultithreadEvent<bool>::EventRevoker _appThemeChangedRevoker;
	::Magpie::Event<uint32_t>::EventRevoker _dpiChangedRevoker;

	com_ptr<implementation::DeleteConfirmationFlyoutContent> _deleteConfirmationFlyoutContent;
	uint32_t _curMenuFlyoutTargetProfileIdx = std::numeric_limits<uint32_t>::max();

	com_ptr<implementation::NewProfileViewModel> _newProfileViewModel = make_self<implementation::NewProfileViewModel>();
	::Magpie::Event<::Magpie::Profile&>::EventRevoker _profileAddedRevoker;
	::Magpie::Event<uint32_t>::EventRevoker _profileRenamedRevoker;
	::Magpie::Event<uint32_t>::EventRevoker _profileRemovedRevoker;
	::Magpie::Event<uint32_t, bool>::EventRevoker _profileMovedRevoker;
	Primitives::FlyoutBase::Opening_revoker _contextFlyoutOpeningRevoker;
};

}
