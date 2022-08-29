#pragma once
#include "ScalingModesPage.g.h"


namespace winrt::Magpie::UI::implementation {

struct ScalingModesPage : ScalingModesPageT<ScalingModesPage> {
	ScalingModesPage();

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

	void AddEffectButton_Click(IInspectable const&, RoutedEventArgs const&);

private:
	void _BuildEffectMenu() noexcept;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingModesPage : ScalingModesPageT<ScalingModesPage, implementation::ScalingModesPage> {
};

}
