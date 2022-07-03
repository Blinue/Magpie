#pragma once
#include "HomePage.g.h"
#include <winrt/Magpie.Runtime.h>
#include "WinRTUtils.h"


namespace winrt::Magpie::App::implementation {

struct HomePage : HomePageT<HomePage> {
	HomePage();

	void AutoRestoreToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void AutoRestoreExpanderToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void CountdownSlider_ValueChanged(IInspectable const&, Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

	void ActivateButton_Click(IInspectable const&, RoutedEventArgs const&);

	void ForgetButton_Click(IInspectable const&, RoutedEventArgs const&);

	void CountdownButton_Click(IInspectable const&, RoutedEventArgs const&);

private:
	void _MagService_IsCountingDownChanged(bool);

	void _MagService_CountdownTick(float value);

	void _MagService_WndToRestoreChanged(uint64_t);

	IAsyncAction _MagRuntime_IsRunningChanged(IInspectable const&, bool value);

	void _UpdateAutoRestoreState();

	void _UpdateDownCount();

	bool _initialized = false;

	Magpie::App::Settings _settings{ nullptr };
	Magpie::Runtime::MagRuntime _magRuntime{ nullptr };

	WinRTUtils::EventRevoker _isCountingDownRevoker;
	WinRTUtils::EventRevoker _countdownTickRevoker;
	WinRTUtils::EventRevoker _wndToRestoreChangedRevoker;
	Magpie::Runtime::MagRuntime::IsRunningChanged_revoker _isRunningChangedRevoker;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HomePage : HomePageT<HomePage, implementation::HomePage> {
};

}
