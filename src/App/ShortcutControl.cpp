#include "pch.h"
#include "ShortcutControl.h"
#if __has_include("ShortcutControl.g.cpp")
#include "ShortcutControl.g.cpp"
#endif

using namespace winrt;


namespace winrt::Magpie::implementation {

ShortcutControl::ShortcutControl() {
	InitializeComponent();

	_shortcutDialog.Title(box_value(L"激活快捷键"));
	_shortcutDialog.Content(_shortcutDialogContent);
	_shortcutDialog.PrimaryButtonText(L"保存");
	_shortcutDialog.CloseButtonText(L"取消");
	_shortcutDialog.DefaultButton(Controls::ContentDialogButton::Primary);

	_hotkeySettings.Win(true);
	_hotkeySettings.Alt(true);
	PreviewKeysControl().ItemsSource(_hotkeySettings.GetKeyList());
}

IAsyncAction ShortcutControl::EditButton_Click(IInspectable const&, RoutedEventArgs const&) {
	_shortcutDialogContent.Keys(_hotkeySettings.GetKeyList());

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

}
