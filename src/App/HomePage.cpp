#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif
#include "StrUtils.h"
#include "Win32Utils.h"
#include "MagService.h"
#include "AppSettings.h"


namespace winrt::Magpie::App::implementation {

HomePage::HomePage() {
	InitializeComponent();

	MagService& magService = MagService::Get();

	_wndToRestoreChangedRevoker = magService.WndToRestoreChanged(
		auto_revoke,
		{ this, &HomePage::_MagService_WndToRestoreChanged}
	);

	AutoRestoreToggleSwitch().IsOn(AppSettings::Get().IsAutoRestore());
	_UpdateAutoRestoreState();
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

void HomePage::ActivateButton_Click(IInspectable const&, RoutedEventArgs const&) {
	HWND wndToRestore = (HWND)MagService::Get().WndToRestore();
	if (wndToRestore) {
		Win32Utils::SetForegroundWindow(wndToRestore);
	}
}

void HomePage::ForgetButton_Click(IInspectable const&, RoutedEventArgs const&) {
	MagService::Get().ClearWndToRestore();
}

void HomePage::_MagService_WndToRestoreChanged(uint64_t) {
	_UpdateAutoRestoreState();
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

}
