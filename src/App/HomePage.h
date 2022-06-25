#pragma once
#include "HomePage.g.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App::implementation {

struct HomePage : HomePageT<HomePage> {
	HomePage();

	void HomePage_Loaded(IInspectable const&, RoutedEventArgs const&);

	void AutoRestoreToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void DownCountSlider_ValueChanged(IInspectable const&, Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

private:
	void _MagService_WndToRestoreChanged(IInspectable const&, uint64_t);

	void _UpdateAutoRestoreState();

	Magpie::App::Settings _settings{ nullptr };
	Magpie::App::MagService _magService{ nullptr };
	Magpie::Runtime::MagRuntime _magRuntime{ nullptr };

	Magpie::App::MagService::WndToRestoreChanged_revoker _wndToRestoreChangedRevoker;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HomePage : HomePageT<HomePage, implementation::HomePage> {
};

}
