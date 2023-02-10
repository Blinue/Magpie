#include "pch.h"
#include "HotkeyControl.h"
#if __has_include("HotkeyControl.g.cpp")
#include "HotkeyControl.g.cpp"
#endif
#include "HotkeyHelper.h"
#include "HotkeyService.h"
#include "AppSettings.h"
#include "XamlUtils.h"
#include "ContentDialogHelper.h"
#include "Logger.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;


namespace winrt::Magpie::App::implementation {

static IVector<IInspectable> ToKeys(const SmallVectorImpl<std::variant<uint8_t, std::wstring>>& keyList) {
	std::vector<IInspectable> result;

	for (const std::variant<uint8_t, std::wstring>& key : keyList) {
		if (key.index() == 0) {
			result.emplace_back(box_value(std::get<0>(key)));
		} else {
			result.emplace_back(box_value(std::get<1>(key)));
		}
	}

	return single_threaded_vector(std::move(result));
}

const DependencyProperty HotkeyControl::ActionProperty = DependencyProperty::Register(
	L"Action",
	xaml_typename<HotkeyAction>(),
	xaml_typename<Magpie::App::HotkeyControl>(),
	PropertyMetadata(box_value(HotkeyAction::COUNT_OR_NONE), &HotkeyControl::_OnActionChanged)
);

const DependencyProperty HotkeyControl::TitleProperty = DependencyProperty::Register(
	L"Title",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::App::HotkeyControl>(),
	PropertyMetadata(box_value(L""), &HotkeyControl::_OnTitleChanged)
);

const DependencyProperty HotkeyControl::_IsErrorProperty = DependencyProperty::Register(
	L"_IsError",
	xaml_typename<bool>(),
	xaml_typename<Magpie::App::HotkeyControl>(),
	PropertyMetadata(box_value(false), nullptr)
);

HotkeyControl* HotkeyControl::_that = nullptr;

HotkeyControl::HotkeyControl() {
	InitializeComponent();

	_hotkeyChangedRevoker = AppSettings::Get().HotkeyChanged(
		auto_revoke, { this,&HotkeyControl::_AppSettings_OnHotkeyChanged });
}

HotkeyControl::~HotkeyControl() {
	if (_keyboardHook) {
		UnhookWindowsHookEx(_keyboardHook);
	}
}

fire_and_forget HotkeyControl::EditButton_Click(IInspectable const&, RoutedEventArgs const&) {
	if (ContentDialogHelper::IsAnyDialogOpen()) {
		co_return;
	}

	if (!_HotkeyDialog) {
		// 惰性初始化
		_HotkeyDialog = ContentDialog();
		_HotkeyDialogContent = HotkeyDialog();

		_HotkeyDialog.Title(GetValue(TitleProperty));
		_HotkeyDialog.Content(_HotkeyDialogContent);
		_HotkeyDialog.PrimaryButtonText(L"保存");
		_HotkeyDialog.CloseButtonText(L"取消");
		_HotkeyDialog.DefaultButton(ContentDialogButton::Primary);
		// 在 Closing 事件中设置热键而不是等待 ShowAsync 返回
		// 这两个时间点有一定间隔，用户在这段时间内的按键不应处理
		_HotkeyDialog.Closing({ this, &HotkeyControl::_HotkeyDialog_Closing });
	}

	_previewHotkey = _hotkey;
	_HotkeyDialogContent.Keys(ToKeys(_previewHotkey.GetKeyList()));

	_HotkeyDialog.XamlRoot(XamlRoot());
	_HotkeyDialog.RequestedTheme(ActualTheme());

	_that = this;
	// 防止钩子冲突
	HotkeyService::Get().StopKeyboardHook();
	_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, _LowLevelKeyboardProc, NULL, 0);
	if (!_keyboardHook) {
		Logger::Get().Win32Error("SetWindowsHookEx 失败");
		HotkeyService::Get().StartKeyboardHook();
		co_return;
	}
	_previewHotkey = _hotkey;
	_HotkeyDialogContent.Keys(ToKeys(_previewHotkey.GetKeyList()));
	_HotkeyDialogContent.Error(IsError() ? _previewHotkey.Check() : HotkeyError::NoError);
	_HotkeyDialog.IsPrimaryButtonEnabled(!IsError());
	
	_pressedKeys.Clear();

	co_await ContentDialogHelper::ShowAsync(_HotkeyDialog);
}

void HotkeyControl::_HotkeyDialog_Closing(ContentDialog const&, ContentDialogClosingEventArgs const& args) {
	UnhookWindowsHookEx(_keyboardHook);
	_keyboardHook = NULL;
	HotkeyService::Get().StartKeyboardHook();

	if (args.Result() == ContentDialogResult::Primary) {
		AppSettings::Get().SetHotkey(Action(), _previewHotkey);
	}
}

LRESULT HotkeyControl::_LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode != HC_ACTION || !_that) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	const DWORD code = ((KBDLLHOOKSTRUCT*)lParam)->vkCode;
	if (code <= 0 || code >= 255) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	// 只有位于前台时才监听按键
	if (GetForegroundWindow() != (HWND)Application::Current().as<App>().HwndMain()) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	bool isKeyDown = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;

	switch (code) {
	case VK_TAB:
		// Tab 键传给系统以移动焦点
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	case VK_LWIN:
	case VK_RWIN:
		_that->_pressedKeys.win = isKeyDown;
		break;
	case VK_CONTROL:
	case VK_LCONTROL:
	case VK_RCONTROL:
		_that->_pressedKeys.ctrl = isKeyDown;
		break;
	case VK_SHIFT:
	case VK_LSHIFT:
	case VK_RSHIFT:
		_that->_pressedKeys.shift = isKeyDown;
		break;
	case VK_MENU:
	case VK_LMENU:
	case VK_RMENU:
		_that->_pressedKeys.alt = isKeyDown;
		break;
	default:
	{
		if (code == VK_RETURN && get_class_name(FocusManager::GetFocusedElement(_that->XamlRoot())) == name_of<Button>()) {
			// 此时用户通过 Tab 键将焦点移到了对话框按钮上
			return CallNextHookEx(NULL, nCode, wParam, lParam);
		}

		if (HotkeyHelper::IsValidKeyCode((uint8_t)code)) {
			if (isKeyDown) {
				_that->_pressedKeys.code = (uint8_t)code;
			} else {
				_that->_pressedKeys.code = 0;
			}
		} else {
			// 不处理的键位
			isKeyDown = false;
		}
		
		break;
	}
	}

	if (isKeyDown) {
		Magpie::App::Hotkey& previewHotkey = _that->_previewHotkey;

		previewHotkey = _that->_pressedKeys;
		_that->_HotkeyDialogContent.Keys(ToKeys(previewHotkey.GetKeyList()));

		HotkeyError error = HotkeyError::NoError;
		bool isPrimaryButtonEnabled = false;
		if (previewHotkey == _that->_hotkey && !_that->IsError()) {
			isPrimaryButtonEnabled = true;
		} else {
			UINT modCount = 0;
			if (previewHotkey.code == 0) {
				if (previewHotkey.win) {
					++modCount;
				}
				if (previewHotkey.alt) {
					++modCount;
				}
				if (modCount <= 1 && previewHotkey.ctrl) {
					++modCount;
				}
				if (modCount <= 1 && previewHotkey.shift) {
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

		_that->_HotkeyDialogContent.Error(error);
		_that->_HotkeyDialog.IsPrimaryButtonEnabled(isPrimaryButtonEnabled);
	}

	return -1;
}

void HotkeyControl::_OnActionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	HotkeyControl* that = get_self<HotkeyControl>(sender.as<default_interface<HotkeyControl>>());
	that->_UpdateHotkey();
}

void HotkeyControl::_OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args) {
	HotkeyControl* that = get_self<HotkeyControl>(sender.as<default_interface<HotkeyControl>>());
	if (that->_HotkeyDialog) {
		that->_HotkeyDialog.Title(args.NewValue());
	}
}

void HotkeyControl::_AppSettings_OnHotkeyChanged(HotkeyAction action) {
	if (action == Action()) {
		_UpdateHotkey();
	}
}

void HotkeyControl::_UpdateHotkey() {
	HotkeyAction action = Action();
	const Hotkey& hotkey = AppSettings::Get().GetHotkey(action);

	_hotkey = hotkey;
	// 此时 HotkeyManager 中的回调已执行
	_IsError(HotkeyService::Get().IsError(action));

	KeysControl().ItemsSource(ToKeys(_hotkey.GetKeyList()));
}

}
