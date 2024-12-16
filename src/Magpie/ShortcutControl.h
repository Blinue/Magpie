#pragma once
#include "ShortcutControl.g.h"
#include "Event.h"
#include "Shortcut.h"

namespace winrt::Magpie::implementation {

struct ShortcutControl : ShortcutControlT<ShortcutControl>, wil::notify_property_changed_base<ShortcutControl> {
	ShortcutControl();

	fire_and_forget EditButton_Click(IInspectable const&, RoutedEventArgs const&);

	ShortcutAction Action() const { return _action; }
	void Action(ShortcutAction value);

	hstring Title() const { return _title; }
	void Title(hstring value);

private:
	void _ShortcutDialog_Closing(Controls::ContentDialog const&, Controls::ContentDialogClosingEventArgs const& args);

	static LRESULT CALLBACK _LowLevelKeyboardProc(
		int    nCode,
		WPARAM wParam,
		LPARAM lParam
	);

	void _AppSettings_OnShortcutChanged(ShortcutAction action);

	void _UpdateShortcut();

	ShortcutAction _action = ShortcutAction::COUNT_OR_NONE;
	hstring _title;

	::Magpie::EventRevoker _shortcutChangedRevoker;

	::Magpie::Shortcut _shortcut;
	Controls::ContentDialog _shortcutDialog{ nullptr };
	Magpie::ShortcutDialog _ShortcutDialogContent{ nullptr };

	wil::unique_hhook _keyboardHook;
	// 用于向键盘钩子传递 this 指针
	// 使用静态成员是一个权宜之计，因为只能同时显示一个弹出窗口
	// 有没有更好的方法？
	static ShortcutControl* _that;

	::Magpie::Shortcut _previewShortcut;
	::Magpie::Shortcut _pressedKeys;

	bool _isError = false;
};

}

namespace winrt::Magpie::factory_implementation {

struct ShortcutControl : ShortcutControlT<ShortcutControl, implementation::ShortcutControl> {
};

}
