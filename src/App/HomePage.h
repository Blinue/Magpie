#pragma once
#include "HomePage.g.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App::implementation {

struct HomePage : HomePageT<HomePage> {
	HomePage();

	void HomePage_Loaded(IInspectable const&, RoutedEventArgs const&);

	void AutoRestoreToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void AutoRestoreExpanderToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&);

	void CountdownSlider_ValueChanged(IInspectable const&, Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

	void ActivateButton_Click(IInspectable const&, RoutedEventArgs const&);

	void ForgetButton_Click(IInspectable const&, RoutedEventArgs const&);

	void CountdownButton_Click(IInspectable const&, RoutedEventArgs const&);

private:
	void _MagService_IsCountingDownChanged(IInspectable const&, bool);

	void _MagService_CountdownTick(IInspectable const&, float value);

	void _MagService_WndToRestoreChanged(IInspectable const&, uint64_t);

	void _Settings_DownCountChanged(IInspectable const&, uint64_t);

	IAsyncAction _MagRuntime_IsRunningChanged(IInspectable const&, bool value);

	void _UpdateAutoRestoreState();

	void _UpdateDownCount();

	Magpie::App::Settings _settings{ nullptr };
	Magpie::App::MagService _magService{ nullptr };
	Magpie::Runtime::MagRuntime _magRuntime{ nullptr };

	Magpie::App::MagService::IsCountingDownChanged_revoker _isCountingDownRevoker;
	Magpie::App::MagService::CountdownTick_revoker _countdownTickRevoker;
	Magpie::App::MagService::WndToRestoreChanged_revoker _wndToRestoreChangedRevoker;
	Magpie::App::Settings::DownCountChanged_revoker _downCountChangedRevoker;
	Magpie::Runtime::MagRuntime::IsRunningChanged_revoker _isRunningChangedRevoker;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HomePage : HomePageT<HomePage, implementation::HomePage> {
};

}
