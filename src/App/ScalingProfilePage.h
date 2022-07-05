#pragma once

#include "ScalingProfilePage.g.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App::implementation {

struct ScalingProfilePage : ScalingProfilePageT<ScalingProfilePage> {
	ScalingProfilePage();

	void OnNavigatedTo(Navigation::NavigationEventArgs const& args);

	Magpie::App::ScalingProfileViewModel ViewModel() const noexcept {
		return _viewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

	void CaptureModeComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&);

	void MultiMonitorUsageComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&);

	void GraphicsAdapterComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&);

	void Is3DGameModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void ShowFPSToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void VSyncToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void TripleBufferingToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void DisableWindowResizingToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void ReserveTitleBarToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void CroppingToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

private:
	void _UpdateVSync();

	Magpie::Runtime::MagSettings _magSettings{ nullptr };
	Magpie::App::ScalingProfileViewModel _viewModel;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ScalingProfilePage : ScalingProfilePageT<ScalingProfilePage, implementation::ScalingProfilePage> {
};

}
