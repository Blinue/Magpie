#pragma once
#include "pch.h"
#include "MainPage.g.h"
#include "WinRTUtils.h"


namespace winrt::Magpie::App {

class ScalingProfile;

}

namespace winrt::Magpie::App::implementation
{
	struct MainPage : MainPageT<MainPage>
	{
		MainPage();

		void Loaded(IInspectable const&, RoutedEventArgs const&);

		void NavigationView_SelectionChanged(MUXC::NavigationView const&, MUXC::NavigationViewSelectionChangedEventArgs const& args);

		void NavigationView_PaneOpening(MUXC::NavigationView const&, IInspectable const&);

		void NavigationView_PaneClosing(MUXC::NavigationView const&, MUXC::NavigationViewPaneClosingEventArgs const&);

		void NavigationView_DisplayModeChanged(MUXC::NavigationView const&, MUXC::NavigationViewDisplayModeChangedEventArgs const&);

		void AddNavigationViewItem_Tapped(IInspectable const&, Input::TappedRoutedEventArgs const&);

		MUXC::NavigationView RootNavigationView();

		void ComboBox_DropDownOpened(IInspectable const&, IInspectable const&);

		event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
			return _propertyChangedEvent.add(handler);
		}

		void PropertyChanged(event_token const& token) noexcept {
			_propertyChangedEvent.remove(token);
		}

		IVector<IInspectable> Profiles() const noexcept {
			return _profiles;
		}

		int32_t ProfileIndex() const noexcept {
			return _profileIndex;
		}

		void ProfileIndex(int32_t value) {
			_profileIndex = value;
			_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ProfileIndex"));
		}

	private:
		void _UpdateTheme(bool updateIcons = true);

		fire_and_forget _LoadIcon(MUXC::NavigationViewItem const& item, const ScalingProfile& profile);

		IAsyncAction _UISettings_ColorValuesChanged(Windows::UI::ViewManagement::UISettings const&, IInspectable const&);

		void _UpdateUWPIcons();

		void _ScalingProfileService_ProfileAdded(ScalingProfile& profile);

		event<PropertyChangedEventHandler> _propertyChangedEvent;

		IVector<IInspectable> _profiles;
		int32_t _profileIndex = 0;

		Windows::UI::ViewManagement::UISettings _uiSettings;
		Windows::UI::ViewManagement::UISettings::ColorValuesChanged_revoker _colorChangedRevoker;

		Controls::ContentDialog _newProfileDialog{ nullptr };

		WinRTUtils::EventRevoker _profileAddedRevoker;

		Windows::Graphics::Display::DisplayInformation _displayInfomation{ nullptr };
	};
}

namespace winrt::Magpie::App::factory_implementation
{
	struct MainPage : MainPageT<MainPage, implementation::MainPage>
	{
	};
}
