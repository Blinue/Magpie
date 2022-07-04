#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif
#include "StrUtils.h"
#include "Win32Utils.h"
#include "MagService.h"
#include "AppSettings.h"


using namespace winrt;
using namespace Windows::UI::Xaml::Controls::Primitives;


namespace winrt::Magpie::App::implementation {

HomePage::HomePage() {
	InitializeComponent();

	App app = Application::Current().as<App>();
	_magRuntime = app.MagRuntime();

	MagService& magService = MagService::Get();

	_isCountingDownRevoker = magService.IsCountingDownChanged(
		auto_revoke,
		{ this, &HomePage::_MagService_IsCountingDownChanged }
	);

	_countdownTickRevoker = magService.CountdownTick(
		auto_revoke,
		{ this, &HomePage::_MagService_CountdownTick }
	);

	_wndToRestoreChangedRevoker = magService.WndToRestoreChanged(
		auto_revoke,
		{ this, &HomePage::_MagService_WndToRestoreChanged}
	);

	_isRunningChangedRevoker = _magRuntime.IsRunningChanged(
		auto_revoke,
		{ this, &HomePage::_MagRuntime_IsRunningChanged }
	);

	AutoRestoreToggleSwitch().IsOn(AppSettings::Get().IsAutoRestore());
	_UpdateAutoRestoreState();

	CountdownButton().IsEnabled(!_magRuntime.IsRunning());
	CountdownSlider().Value(AppSettings::Get().DownCount());
	_UpdateDownCount();
}

void HomePage::AutoRestoreToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	if (!IsLoaded()) {
		return;
	}

	bool isOn = AutoRestoreToggleSwitch().IsOn();
	AppSettings::Get().IsAutoRestore(isOn);

	if (!isOn) {
		AutoRestoreSettingItem().Visibility(Visibility::Visible);
		AutoRestoreExpander().Visibility(Visibility::Collapsed);
	}
}

void HomePage::AutoRestoreExpanderToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	if (!IsLoaded()) {
		return;
	}

	AutoRestoreToggleSwitch().IsOn(false);
	AutoRestoreExpanderToggleSwitch().IsOn(true);
}

void HomePage::CountdownSlider_ValueChanged(IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
	if (!IsLoaded()) {
		return;
	}

	AppSettings::Get().DownCount((uint32_t)args.NewValue());
	_UpdateDownCount();
}

void HomePage::ActivateButton_Click(IInspectable const&, RoutedEventArgs const&) {
	HWND wndToRestore = (HWND)MagService::Get().WndToRestore();
	if (wndToRestore) {
		Win32Utils::SetForegroundWindow(wndToRestore);
	}
}

void HomePage::ForgetButton_Click(IInspectable const&, RoutedEventArgs const&) {
	MagService::Get().ClearWndToRestore();
}

void HomePage::CountdownButton_Click(IInspectable const&, RoutedEventArgs const&) {
	if (MagService::Get().IsCountingDown()) {
		MagService::Get().StopCountdown();
	} else {
		MagService::Get().StartCountdown();
	}
}

void HomePage::_MagService_IsCountingDownChanged(bool) {
	_UpdateDownCount();
}

void HomePage::_MagService_CountdownTick(float value) {
	CountdownProgressRing().Value(value / MagService::Get().TickingDownCount());
	CountdownTextBlock().Text(std::to_wstring((int)std::ceil(value)));
}

void HomePage::_MagService_WndToRestoreChanged(uint64_t) {
	_UpdateAutoRestoreState();
}

IAsyncAction HomePage::_MagRuntime_IsRunningChanged(IInspectable const&, bool value) {
	co_await Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [&, value]() {
		CountdownButton().IsEnabled(!value);
	});
}

void HomePage::_UpdateAutoRestoreState() {
	const MUXC::Expander& autoRestoreExpander = AutoRestoreExpander();

	HWND wndToRestore = (HWND)MagService::Get().WndToRestore();
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
	if (MagService::Get().IsCountingDown()) {
		CountdownVisual().Visibility(Visibility::Visible);
		CountdownButton().Content(box_value(L"取消"));

		_MagService_CountdownTick(MagService::Get().CountdownLeft());
	} else {
		CountdownVisual().Visibility(Visibility::Collapsed);
		CountdownButton().Content(box_value(fmt::format(L"{} 秒后缩放", AppSettings::Get().DownCount())));

		// 修复动画错误
		CountdownProgressRing().Value(1.0f);
	}
}

}
