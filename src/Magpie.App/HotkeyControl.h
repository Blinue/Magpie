#pragma once
#include "HotkeyControl.g.h"
#include "WinRTUtils.h"
#include "Hotkey.h"

namespace winrt::Magpie::App::implementation {

struct HotkeyControl : HotkeyControlT<HotkeyControl> {
	HotkeyControl();
	~HotkeyControl();

	fire_and_forget EditButton_Click(IInspectable const&, RoutedEventArgs const&);

	HotkeyAction Action() const {
		return GetValue(ActionProperty).as<HotkeyAction>();
	}

	void Action(HotkeyAction value) {
		SetValue(ActionProperty, box_value(value));
	}

	hstring Title() const {
		return GetValue(TitleProperty).as<hstring>();
	}

	void Title(const hstring& value) {
		SetValue(TitleProperty, box_value(value));
	}

	bool IsError() const {
		return GetValue(_IsErrorProperty).as<bool>();
	}

	static const DependencyProperty ActionProperty;
	static const DependencyProperty TitleProperty;

private:
	void _HotkeyDialog_Closing(Controls::ContentDialog const&, Controls::ContentDialogClosingEventArgs const& args);

	static LRESULT CALLBACK _LowLevelKeyboardProc(
		int    nCode,
		WPARAM wParam,
		LPARAM lParam
	);

	static const DependencyProperty _IsErrorProperty;
	
	static void _OnActionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	static void _OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args);

	void _AppSettings_OnHotkeyChanged(HotkeyAction action);

	void _UpdateHotkey();

	void _IsError(bool value) {
		SetValue(_IsErrorProperty, box_value(value));
	}

	WinRTUtils::EventRevoker _hotkeyChangedRevoker;

	Hotkey _hotkey;
	Controls::ContentDialog _HotkeyDialog{ nullptr };
	Magpie::App::HotkeyDialog _HotkeyDialogContent{ nullptr };

	HHOOK _keyboardHook = NULL;
	// 用于向键盘钩子传递 this 指针
	// 使用静态成员是一个权宜之计，因为只能同时显示一个弹出窗口
	// 有没有更好的方法？
	static HotkeyControl* _that;

	Hotkey _previewHotkey;
	Hotkey _pressedKeys;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HotkeyControl : HotkeyControlT<HotkeyControl, implementation::HotkeyControl> {
};

}
