#pragma once
#include "ScalingModesPage.g.h"


namespace winrt::Magpie::UI::implementation {

struct ScalingModesPage : ScalingModesPageT<ScalingModesPage> {
	ScalingModesPage();

	Magpie::UI::ScalingModesViewModel ViewModel() const noexcept {
		return _viewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

	void AddEffectButton_Click(IInspectable const& sender, RoutedEventArgs const&);

	void NewScalingModeFlyout_Opening(IInspectable const&, IInspectable const&);

	void NewScalingModeNameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args);

	void NewScalingModeConfirmButton_Click(IInspectable const& sender, RoutedEventArgs const&);

private:
	void _BuildEffectMenu() noexcept;

	Magpie::UI::ScalingModesViewModel _viewModel;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingModesPage : ScalingModesPageT<ScalingModesPage, implementation::ScalingModesPage> {
};

}
