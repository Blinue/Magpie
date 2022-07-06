#pragma once

#include "pch.h"
#include "ShortcutControl.g.h"
#include "WinRTUtils.h"
#include "HotkeySettings.h"


namespace winrt::Magpie::App::implementation {

struct ShortcutControl : ShortcutControlT<ShortcutControl> {
	ShortcutControl();

	IAsyncAction EditButton_Click(IInspectable const&, RoutedEventArgs const&);

	void ShortcutDialog_Opened(Controls::ContentDialog const&, Controls::ContentDialogOpenedEventArgs const&);
	void ShortcutDialog_Closing(Controls::ContentDialog const&, Controls::ContentDialogClosingEventArgs const& args);

	HotkeyAction Action() const;
	void Action(HotkeyAction value);

	bool IsError() const;

	static const DependencyProperty ActionProperty;

private:
	static LRESULT CALLBACK _LowLevelKeyboardProc(
		int    nCode,
		WPARAM wParam,
		LPARAM lParam
	);

	static const DependencyProperty _IsErrorProperty;
	
	static void _OnActionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _Settings_OnHotkeyChanged(HotkeyAction action);

	void _UpdateHotkey();

	void _IsError(bool value);

	WinRTUtils::EventRevoker _hotkeyChangedRevoker;

	Magpie::App::HotkeySettings _hotkey;
	Controls::ContentDialog _shortcutDialog;
	Magpie::App::ShortcutDialog _shortcutDialogContent;

	HHOOK _keyboardHook = NULL;
	// 用于向键盘钩子传递 this 指针
	// 使用静态成员是一个权宜之计，因为只能同时显示一个弹出窗口
	// 有没有更好的方法？
	static ShortcutControl* _that;

	HotkeySettings _previewHotkey;
	HotkeySettings _pressedKeys;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ShortcutControl : ShortcutControlT<ShortcutControl, implementation::ShortcutControl> {
};

}
