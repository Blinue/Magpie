#pragma once

#include "pch.h"
#include "ShortcutControl.g.h"


namespace winrt::Magpie::App::implementation {

struct ShortcutControl : ShortcutControlT<ShortcutControl> {
	ShortcutControl();

	IAsyncAction EditButton_Click(IInspectable const&, RoutedEventArgs const&);

	void ShortcutDialog_Opened(Controls::ContentDialog const&, Controls::ContentDialogOpenedEventArgs const&);
	void ShortcutDialog_Closing(Controls::ContentDialog const&, Controls::ContentDialogClosingEventArgs const& args);

	bool IsError() const;

	event_token PropertyChanged(Data::PropertyChangedEventHandler const& value);
	void PropertyChanged(event_token const& token);

private:
	static LRESULT CALLBACK _LowLevelKeyboardProc(
		int    nCode,
		WPARAM wParam,
		LPARAM lParam
	);

	static const DependencyProperty _IsErrorProperty;
	static void _OnIsErrorChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _IsError(bool value);

	event<Data::PropertyChangedEventHandler> _propertyChangedEvent;

	Magpie::App::HotkeySettings _hotkey;
	Controls::ContentDialog _shortcutDialog;
	Magpie::App::ShortcutDialogContent _shortcutDialogContent;

	HHOOK _keyboardHook = NULL;
	// 用于向键盘钩子传递 this 指针
	// 使用静态成员是一个权宜之计，因为只能同时显示一个弹出窗口
	// 有没有更好的方法？
	static ShortcutControl* _that;

	Magpie::App::HotkeySettings _previewHotkey;
	Magpie::App::HotkeySettings _pressedKeys;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ShortcutControl : ShortcutControlT<ShortcutControl, implementation::ShortcutControl> {
};

}
