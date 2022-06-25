#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif
#include "StrUtils.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Controls::Primitives;


namespace winrt::Magpie::App::implementation {

HomePage::HomePage() {
	InitializeComponent();

	App app = Application::Current().as<App>();
	_settings = app.Settings();
	_magService = app.MagService();
	_magRuntime = app.MagRuntime();

	_wndToRestoreChangedRevoker = _magService.WndToRestoreChanged(
		auto_revoke,
		{ this, &HomePage::_MagService_WndToRestoreChanged}
	);

	AutoRestoreToggleSwitch().IsOn(_settings.IsAutoRestore());
	DownCountSlider().Value(_settings.DownCount());
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

void HomePage::DownCountSlider_ValueChanged(IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
	if (_settings) {
		_settings.DownCount((uint32_t)args.NewValue());
	}
}

void HomePage::_MagService_WndToRestoreChanged(IInspectable const&, uint64_t) {
	_UpdateAutoRestoreState();
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

}
