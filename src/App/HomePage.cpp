#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif
#include "StrUtils.h"
#include "Win32Utils.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Controls::Primitives;


namespace winrt::Magpie::App::implementation {

HomePage::HomePage() {
	InitializeComponent();

	App app = Application::Current().as<App>();
	_settings = app.Settings();
	_magService = app.MagService();
	_magRuntime = app.MagRuntime();

	_isCountingDownRevoker = _magService.IsCountingDownChanged(
		auto_revoke,
		{ this, &HomePage::_MagService_IsCountingDownChanged }
	);

	_countdownTickRevoker = _magService.CountdownTick(
		auto_revoke,
		{ this, &HomePage::_MagService_CountdownTick }
	);

	_wndToRestoreChangedRevoker = _magService.WndToRestoreChanged(
		auto_revoke,
		{ this, &HomePage::_MagService_WndToRestoreChanged}
	);

	_downCountChangedRevoker = _settings.DownCountChanged(
		auto_revoke,
		{ this, &HomePage::_Settings_DownCountChanged}
	);

	_isRunningChangedRevoker = _magRuntime.IsRunningChanged(
		auto_revoke,
		{ this, &HomePage::_MagRuntime_IsRunningChanged }
	);

	AutoRestoreToggleSwitch().IsOn(_settings.IsAutoRestore());
	_UpdateDownCount();
}

void HomePage::HomePage_Loaded(IInspectable const&, RoutedEventArgs const&) {
	_UpdateAutoRestoreState();
}

void HomePage::AutoRestoreToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	if (_settings) {
		bool isOn = AutoRestoreToggleSwitch().IsOn();
		_settings.IsAutoRestore(isOn);

		if (!isOn) {
			AutoRestoreSettingItem().Visibility(Visibility::Visible);
			AutoRestoreExpander().Visibility(Visibility::Collapsed);
		}
	}
}

void HomePage::AutoRestoreExpanderToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	if (_settings) {
		AutoRestoreToggleSwitch().IsOn(false);
		AutoRestoreExpanderToggleSwitch().IsOn(true);
	}
}

void HomePage::CountdownSlider_ValueChanged(IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
	if (_settings) {
		_settings.DownCount((uint32_t)args.NewValue());
	}
}

void HomePage::ActivateButton_Click(IInspectable const&, RoutedEventArgs const&) {
	HWND wndToRestore = (HWND)_magService.WndToRestore();
	if (wndToRestore) {
		Win32Utils::SetForegroundWindow(wndToRestore);
	}
}

void HomePage::ForgetButton_Click(IInspectable const&, RoutedEventArgs const&) {
	_magService.ClearWndToRestore();
}

void HomePage::CountdownButton_Click(IInspectable const&, RoutedEventArgs const&) {
	if (_magService.IsCountingDown()) {
		_magService.StopCountdown();
	} else {
		_magService.StartCountdown();
	}
}

void HomePage::_MagService_IsCountingDownChanged(IInspectable const&, bool value) {
	if (value) {
		CountdownVisual().Visibility(Visibility::Visible);
		CountdownButton().Content(box_value(L"取消"));
	} else {
		CountdownVisual().Visibility(Visibility::Collapsed);
		CountdownButton().Content(box_value(fmt::format(L"{} 秒后缩放", _settings.DownCount())));
	}

	_MagService_CountdownTick(nullptr, (float)_magService.TickingDownCount());
}

void HomePage::_MagService_CountdownTick(IInspectable const&, float value) {
	CountdownProgressRing().Value(value / _magService.TickingDownCount());
	CountdownTextBlock().Text(std::to_wstring((int)std::ceil(value)));
}

void HomePage::_MagService_WndToRestoreChanged(IInspectable const&, uint64_t) {
	_UpdateAutoRestoreState();
}

void HomePage::_Settings_DownCountChanged(IInspectable const&, uint64_t) {
	_UpdateDownCount();
}

IAsyncAction HomePage::_MagRuntime_IsRunningChanged(IInspectable const&, bool value) {
	co_await Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [&, value]() {
		CountdownButton().IsEnabled(!value);
	});
}

void HomePage::_UpdateAutoRestoreState() {
	const MUXC::Expander& autoRestoreExpander = AutoRestoreExpander();

	HWND wndToRestore = (HWND)_magService.WndToRestore();
	if (wndToRestore) {
		AutoRestoreSettingItem().Visibility(Visibility::Collapsed);
		autoRestoreExpander.Visibility(Visibility::Visible);
		autoRestoreExpander.IsExpanded(true);

		std::wstring title(GetWindowTextLength(wndToRestore), L'\0');
		GetWindowText(wndToRestore, title.data(), (int)title.size() + 1);

		AutoRestoreExpandedSettingItem().Title(
			StrUtils::ConcatW(L"当前窗口：", title.empty() ? L"<标题为空>" : title));
	} else {
		AutoRestoreSettingItem().Visibility(Visibility::Visible);
		autoRestoreExpander.Visibility(Visibility::Collapsed);
		autoRestoreExpander.IsExpanded(false);
	}
}

void HomePage::_UpdateDownCount() {
	uint32_t downCount = _settings.DownCount();
	CountdownSlider().Value(downCount);
	if (!_magService.IsCountingDown()) {
		CountdownButton().Content(box_value(fmt::format(L"{} 秒后缩放", downCount)));
	}
}

}
