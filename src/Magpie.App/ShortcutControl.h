#pragma once
#include "ShortcutControl.g.h"
#include "WinRTUtils.h"
#include "Shortcut.h"

namespace winrt::Magpie::App::implementation {

struct ShortcutControl : ShortcutControlT<ShortcutControl> {
	ShortcutControl();
	~ShortcutControl();

	fire_and_forget EditButton_Click(IInspectable const&, RoutedEventArgs const&);

	ShortcutAction Action() const {
		return GetValue(ActionProperty).as<ShortcutAction>();
	}

	void Action(ShortcutAction value) {
		SetValue(ActionProperty, box_value(value));
	}

	hstring Title() const {
		return GetValue(TitleProperty).as<hstring>();
	}

	void Title(const hstring& value) {
		SetValue(TitleProperty, box_value(value));
	}

	static const DependencyProperty ActionProperty;
	static const DependencyProperty TitleProperty;

private:
	void _ShortcutDialog_Closing(Controls::ContentDialog const&, Controls::ContentDialogClosingEventArgs const& args);

	static LRESULT CALLBACK _LowLevelKeyboardProc(
		int    nCode,
		WPARAM wParam,
		LPARAM lParam
	);
	
	static void _OnActionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	static void _OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args);

	void _AppSettings_OnShortcutChanged(ShortcutAction action);

	void _UpdateShortcut();

	WinRTUtils::EventRevoker _shortcutChangedRevoker;

	Shortcut _shortcut;
	Controls::ContentDialog _ShortcutDialog{ nullptr };
	Magpie::App::ShortcutDialog _ShortcutDialogContent{ nullptr };

	HHOOK _keyboardHook = NULL;
	// 用于向键盘钩子传递 this 指针
	// 使用静态成员是一个权宜之计，因为只能同时显示一个弹出窗口
	// 有没有更好的方法？
	static ShortcutControl* _that;

	Shortcut _previewShortcut;
	Shortcut _pressedKeys;

	bool _isError = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ShortcutControl : ShortcutControlT<ShortcutControl, implementation::ShortcutControl> {
};

}
