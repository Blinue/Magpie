#include "pch.h"
#include "ShortcutControl.h"
#if __has_include("ShortcutControl.g.cpp")
#include "ShortcutControl.g.cpp"
#endif
#include "HotkeyHelper.h"
#include "HotkeyService.h"


using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;


namespace winrt::Magpie::App::implementation {

const DependencyProperty ShortcutControl::ActionProperty = DependencyProperty::Register(
	L"Action",
	xaml_typename<HotkeyAction>(),
	xaml_typename<Magpie::App::ShortcutControl>(),
	PropertyMetadata(box_value(HotkeyAction::COUNT_OR_NONE), &ShortcutControl::_OnActionChanged)
);

const DependencyProperty ShortcutControl::_IsErrorProperty = DependencyProperty::Register(
	L"_IsError",
	xaml_typename<bool>(),
	xaml_typename<Magpie::App::ShortcutControl>(),
	PropertyMetadata(box_value(false), nullptr)
);

ShortcutControl* ShortcutControl::_that = nullptr;

ShortcutControl::ShortcutControl() {
	InitializeComponent();

	Magpie::App::App app = Application::Current().as<Magpie::App::App>();
	_settings = app.Settings();

	_hotkeyChangedRevoker = _settings.HotkeyChanged(
		auto_revoke, { this,&ShortcutControl::_Settings_OnHotkeyChanged });

	_shortcutDialog.Title(box_value(L"激活快捷键"));
	_shortcutDialog.Content(_shortcutDialogContent);
	_shortcutDialog.PrimaryButtonText(L"保存");
	_shortcutDialog.CloseButtonText(L"取消");
	_shortcutDialog.DefaultButton(ContentDialogButton::Primary);
	_shortcutDialog.Opened({ this, &ShortcutControl::ShortcutDialog_Opened });
	_shortcutDialog.Closing({ this, &ShortcutControl::ShortcutDialog_Closing });
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
	_previewHotkey.CopyFrom(_hotkey);
	_shortcutDialogContent.Keys(_previewHotkey.GetKeyList());
	_shortcutDialogContent.Error(IsError() ? _previewHotkey.Check() : HotkeyError::NoError);
	_shortcutDialog.IsPrimaryButtonEnabled(!IsError());

	_pressedKeys.Clear();
}

void ShortcutControl::ShortcutDialog_Closing(ContentDialog const&, ContentDialogClosingEventArgs const& args) {
	UnhookWindowsHookEx(_keyboardHook);

	if (args.Result() == ContentDialogResult::Primary) {
		_settings.SetHotkey(Action(), _previewHotkey);
	}
}

HotkeyAction ShortcutControl::Action() const {
	return GetValue(ActionProperty).as<HotkeyAction>();
}

void ShortcutControl::Action(HotkeyAction value) {
	SetValue(ActionProperty, box_value(value));
}

void ShortcutControl::_IsError(bool value) {
	SetValue(_IsErrorProperty, box_value(value));
}

bool ShortcutControl::IsError() const {
	return GetValue(_IsErrorProperty).as<bool>();
}

LRESULT ShortcutControl::_LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode != HC_ACTION || !_that) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	// 只有位于前台时才监听按键
	Magpie::App::App app = Application::Current().as<Magpie::App::App>();
	if (GetForegroundWindow() != (HWND)app.HwndHost()) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	bool isKeyDown = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;

	DWORD code = ((KBDLLHOOKSTRUCT*)lParam)->vkCode;
	switch (code) {
	case VK_TAB:
		// Tab 键传给系统以移动焦点
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	case VK_LWIN:
	case VK_RWIN:
		_that->_pressedKeys.Win(isKeyDown);
		break;
	case VK_CONTROL:
	case VK_LCONTROL:
	case VK_RCONTROL:
		_that->_pressedKeys.Ctrl(isKeyDown);
		break;
	case VK_SHIFT:
	case VK_LSHIFT:
	case VK_RSHIFT:
		_that->_pressedKeys.Shift(isKeyDown);
		break;
	case VK_MENU:
	case VK_LMENU:
	case VK_RMENU:
		_that->_pressedKeys.Alt(isKeyDown);
		break;
	default:
	{
		if (code == VK_RETURN && get_class_name(FocusManager::GetFocusedElement(_that->XamlRoot())) == name_of<Button>()) {
			// 此时用户通过 Tab 键将焦点移到了对话框按钮上
			return CallNextHookEx(NULL, nCode, wParam, lParam);
		}

		if (HotkeyHelper::IsValidKeyCode(code)) {
			if (isKeyDown) {
				_that->_pressedKeys.Code(code);
			} else {
				_that->_pressedKeys.Code(0);
			}
		} else {
			// 不处理的键位
			isKeyDown = false;
		}
		
		break;
	}
	}

	if (isKeyDown) {
		Magpie::App::HotkeySettings& previewHotkey = _that->_previewHotkey;

		previewHotkey.CopyFrom(_that->_pressedKeys);
		_that->_shortcutDialogContent.Keys(previewHotkey.GetKeyList());

		HotkeyError error = HotkeyError::NoError;
		bool isPrimaryButtonEnabled = false;
		if (previewHotkey.Equals(_that->_hotkey) && !_that->IsError()) {
			isPrimaryButtonEnabled = true;
		} else {
			UINT modCount = 0;
			if (previewHotkey.Code() == 0) {
				if (previewHotkey.Win()) {
					++modCount;
				}
				if (previewHotkey.Alt()) {
					++modCount;
				}
				if (modCount <= 1 && previewHotkey.Ctrl()) {
					++modCount;
				}
				if (modCount <= 1 && previewHotkey.Shift()) {
					++modCount;
				}
			}

			if (modCount == 1) {
				// Modifiers 个数为 1 时不显示错误
				isPrimaryButtonEnabled = false;
			} else {
				error = previewHotkey.Check();
				isPrimaryButtonEnabled = error == HotkeyError::NoError;
			}
		}

		_that->_shortcutDialogContent.Error(error);
		_that->_shortcutDialog.IsPrimaryButtonEnabled(isPrimaryButtonEnabled);
	}

	return -1;
}

void ShortcutControl::_OnActionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	ShortcutControl* that = get_self<ShortcutControl>(sender.as<default_interface<ShortcutControl>>());
	that->_UpdateHotkey();
}

void ShortcutControl::_Settings_OnHotkeyChanged(IInspectable const&, HotkeyAction action) {
	if (action == Action()) {
		_UpdateHotkey();
	}
}

void ShortcutControl::_UpdateHotkey() {
	HotkeyAction action = Action();
	HotkeySettings hotkey = _settings.GetHotkey(action);
	if (hotkey) {
		_hotkey.CopyFrom(hotkey);
		// 此时 HotkeyManager 中的回调已执行
		_IsError(HotkeyService::Get().IsError(action));
	} else {
		_hotkey.Clear();
		_IsError(false);
	}

	KeysControl().ItemsSource(_hotkey.GetKeyList());
}

}
