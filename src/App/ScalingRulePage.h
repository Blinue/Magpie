#pragma once

#include "ScalingRulePage.g.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App::implementation {

struct ScalingRulePage : ScalingRulePageT<ScalingRulePage> {
	ScalingRulePage();

	Magpie::App::ScalingRuleViewModel ViewModel() const noexcept {
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
	Magpie::App::ScalingRuleViewModel _viewModel;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ScalingRulePage : ScalingRulePageT<ScalingRulePage, implementation::ScalingRulePage> {
};

}
