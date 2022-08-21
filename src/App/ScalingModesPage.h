#pragma once
#include "ScalingModesPage.g.h"


namespace winrt::Magpie::App::implementation {

struct ScalingModesPage : ScalingModesPageT<ScalingModesPage> {
	ScalingModesPage();

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

	void AddEffectButton_Click(IInspectable const&, RoutedEventArgs const&);
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ScalingModesPage : ScalingModesPageT<ScalingModesPage, implementation::ScalingModesPage> {
};

}
