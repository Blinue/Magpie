#pragma once
#include "HomePage.g.h"


namespace winrt::Magpie::App::implementation {

struct HomePage : HomePageT<HomePage> {
	HomePage();

	void AutoRestoreToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void DownCountSlider_ValueChanged(IInspectable const&, Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

private:
	Magpie::App::Settings _settings{ nullptr };
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HomePage : HomePageT<HomePage, implementation::HomePage> {
};

}
