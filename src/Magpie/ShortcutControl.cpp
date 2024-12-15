#include "pch.h"
#include "ShortcutControl.h"
#if __has_include("ShortcutControl.g.cpp")
#include "ShortcutControl.g.cpp"
#endif
#include "ShortcutHelper.h"
#include "ShortcutService.h"
#include "AppSettings.h"
#include "XamlHelper.h"
#include "ContentDialogHelper.h"
#include "Logger.h"
#include "CommonSharedConstants.h"
#include "App.h"

using namespace Magpie;
using namespace Magpie::Core;
using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;

namespace winrt::Magpie::implementation {

static IVector<IInspectable> ToKeys(const Shortcut& shortcut, bool isError) {
	std::vector<IInspectable> result;

	if (shortcut.win) {
		result.push_back(KeyVisualState(VK_LWIN, isError));
	}
	if (shortcut.ctrl) {
		result.push_back(KeyVisualState(VK_LCONTROL, isError));
	}
	if (shortcut.alt) {
		result.push_back(KeyVisualState(VK_LMENU, isError));
	}
	if (shortcut.shift) {
		result.push_back(KeyVisualState(VK_LSHIFT, isError));
	}
	if (shortcut.code) {
		result.push_back(KeyVisualState((int)shortcut.code, isError));
	}

	return single_threaded_vector(std::move(result));
}

const DependencyProperty ShortcutControl::ActionProperty = DependencyProperty::Register(
	L"Action",
	xaml_typename<ShortcutAction>(),
	xaml_typename<class_type>(),
	PropertyMetadata(box_value(ShortcutAction::COUNT_OR_NONE), &ShortcutControl::_OnActionChanged)
);

const DependencyProperty ShortcutControl::TitleProperty = DependencyProperty::Register(
	L"Title",
	xaml_typename<hstring>(),
	xaml_typename<class_type>(),
	PropertyMetadata(box_value(L""), &ShortcutControl::_OnTitleChanged)
);

ShortcutControl* ShortcutControl::_that = nullptr;

ShortcutControl::ShortcutControl() {
	_shortcutChangedRevoker = AppSettings::Get().ShortcutChanged(
		auto_revoke, { this,&ShortcutControl::_AppSettings_OnShortcutChanged });
}

fire_and_forget ShortcutControl::EditButton_Click(IInspectable const&, RoutedEventArgs const&) {
	if (ContentDialogHelper::IsAnyDialogOpen()) {
		co_return;
	}

	if (!_shortcutDialog) {
		// 惰性初始化
		_shortcutDialog = ContentDialog();
		_ShortcutDialogContent = ShortcutDialog();

		// 设置 Language 属性帮助 XAML 选择合适的字体
		_shortcutDialog.Language(Language());
		_shortcutDialog.Title(GetValue(TitleProperty));
		_shortcutDialog.Content(_ShortcutDialogContent);
		ResourceLoader resourceLoader =
			ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
		_shortcutDialog.PrimaryButtonText(resourceLoader.GetString(L"ShortcutDialog_Save"));
		_shortcutDialog.CloseButtonText(resourceLoader.GetString(L"ShortcutDialog_Cancel"));
		_shortcutDialog.DefaultButton(ContentDialogButton::Primary);
		// 在 Closing 事件中设置热键而不是等待 ShowAsync 返回
		// 这两个时间点有一定间隔，用户在这段时间内的按键不应处理
		_shortcutDialog.Closing({ this, &ShortcutControl::_ShortcutDialog_Closing });
	}

	_shortcutDialog.XamlRoot(XamlRoot());
	_shortcutDialog.RequestedTheme(ActualTheme());

	_that = this;
	// 防止钩子冲突
	ShortcutService::Get().StopKeyboardHook();
	_keyboardHook.reset(SetWindowsHookEx(WH_KEYBOARD_LL, _LowLevelKeyboardProc, NULL, 0));
	if (!_keyboardHook) {
		Logger::Get().Win32Error("SetWindowsHookEx 失败");
		ShortcutService::Get().StartKeyboardHook();
		co_return;
	}
	_previewShortcut = _shortcut;
	
	ShortcutError error = _isError ? ShortcutHelper::CheckShortcut(_previewShortcut) : ShortcutError::NoError;
	_ShortcutDialogContent.Keys(ToKeys(_previewShortcut, error != ShortcutError::NoError));
	_ShortcutDialogContent.Error(error);
	_shortcutDialog.IsPrimaryButtonEnabled(error == ShortcutError::NoError);
	
	_pressedKeys.Clear();

	co_await ContentDialogHelper::ShowAsync(_shortcutDialog);
}

void ShortcutControl::_ShortcutDialog_Closing(ContentDialog const&, ContentDialogClosingEventArgs const& args) {
	_keyboardHook.reset();
	ShortcutService::Get().StartKeyboardHook();

	if (args.Result() == ContentDialogResult::Primary) {
		AppSettings::Get().SetShortcut(Action(), _previewShortcut);
	}
}

LRESULT ShortcutControl::_LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode != HC_ACTION || !_that) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	const DWORD code = ((KBDLLHOOKSTRUCT*)lParam)->vkCode;
	if (code <= 0 || code >= 255) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	// 只有位于前台时才监听按键
	if (GetForegroundWindow() != App::Get().MainWindow().Handle()) {
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

		if (ShortcutHelper::IsValidKeyCode((uint8_t)code)) {
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
		Shortcut& previewShortcut = _that->_previewShortcut;

		previewShortcut = _that->_pressedKeys;

		ShortcutError error = ShortcutError::NoError;
		bool isPrimaryButtonEnabled = false;
		if (previewShortcut == _that->_shortcut && !_that->_isError) {
			isPrimaryButtonEnabled = true;
		} else {
			UINT modCount = 0;
			if (previewShortcut.code == 0) {
				if (previewShortcut.win) {
					++modCount;
				}
				if (previewShortcut.alt) {
					++modCount;
				}
				if (modCount <= 1 && previewShortcut.ctrl) {
					++modCount;
				}
				if (modCount <= 1 && previewShortcut.shift) {
					++modCount;
				}
			}

			if (modCount == 1) {
				// Modifiers 个数为 1 时不显示错误
				isPrimaryButtonEnabled = false;
			} else {
				error = ShortcutHelper::CheckShortcut(previewShortcut);
				isPrimaryButtonEnabled = error == ShortcutError::NoError;
			}
		}

		_that->_ShortcutDialogContent.Keys(ToKeys(previewShortcut, error != ShortcutError::NoError));
		_that->_ShortcutDialogContent.Error(error);
		_that->_shortcutDialog.IsPrimaryButtonEnabled(isPrimaryButtonEnabled);
	}

	return -1;
}

void ShortcutControl::_OnActionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	ShortcutControl* that = get_self<ShortcutControl>(sender.as<class_type>());
	that->_UpdateShortcut();
}

void ShortcutControl::_OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args) {
	ShortcutControl* that = get_self<ShortcutControl>(sender.as<class_type>());
	if (that->_shortcutDialog) {
		that->_shortcutDialog.Title(args.NewValue());
	}
}

void ShortcutControl::_AppSettings_OnShortcutChanged(ShortcutAction action) {
	if (action == Action()) {
		_UpdateShortcut();
	}
}

void ShortcutControl::_UpdateShortcut() {
	ShortcutAction action = Action();
	const Shortcut& shortcut = AppSettings::Get().GetShortcut(action);

	_shortcut = shortcut;
	// 此时 ShortcutService 中的回调已执行
	_isError = ShortcutService::Get().IsError(action);
	KeysControl().ItemsSource(ToKeys(_shortcut, _isError));
}

}
