#include "pch.h"
#include "ShortcutControl.h"
#if __has_include("ShortcutControl.g.cpp")
#include "ShortcutControl.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;


namespace winrt::Magpie::App::implementation {

ShortcutControl* ShortcutControl::_that = nullptr;

ShortcutControl::ShortcutControl() {
	InitializeComponent();

	_shortcutDialog.Title(box_value(L"激活快捷键"));
	_shortcutDialog.Content(_shortcutDialogContent);
	_shortcutDialog.PrimaryButtonText(L"保存");
	_shortcutDialog.CloseButtonText(L"取消");
	_shortcutDialog.DefaultButton(ContentDialogButton::Primary);
	_shortcutDialog.Opened({ this, &ShortcutControl::ShortcutDialog_Opened });
	_shortcutDialog.Closing({ this, &ShortcutControl::ShortcutDialog_Closing });

	_hotkey.Win(true);
	_hotkey.Alt(true);
	KeysControl().ItemsSource(_hotkey.GetKeyList());
}

IAsyncAction ShortcutControl::EditButton_Click(IInspectable const&, RoutedEventArgs const&) {
	_previewHotkey.CopyFrom(_hotkey);
	_shortcutDialogContent.Keys(_previewHotkey.GetKeyList());

	_shortcutDialog.XamlRoot(XamlRoot());
	_shortcutDialog.RequestedTheme(ActualTheme());

	// 防止快速点击时崩溃
	static bool showing = false;
	if (showing) {
		co_return;
	}
	showing = true;
	co_await _shortcutDialog.ShowAsync();
	showing = false;
}

void ShortcutControl::ShortcutDialog_Opened(ContentDialog const&, ContentDialogOpenedEventArgs const&) {
	_that = this;
	_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, _LowLevelKeyboardProc, NULL, 0);
}

void ShortcutControl::ShortcutDialog_Closing(ContentDialog const&, ContentDialogClosingEventArgs const&) {
	UnhookWindowsHookEx(_keyboardHook);
	_hotkey.CopyFrom(_previewHotkey);
	KeysControl().ItemsSource(_hotkey.GetKeyList());
}

LRESULT ShortcutControl::_LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode != HC_ACTION || !_that) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	// 只有位于前台时才监听按键
	App app = Application::Current().as<App>();
	if (GetForegroundWindow() != (HWND)app.HwndHost()) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
		OutputDebugString(L"down");
	} else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
		OutputDebugString(L"up");
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

}
