#include "pch.h"
#include "ShortcutService.h"
#include "Logger.h"
#include "ShortcutHelper.h"
#include "AppSettings.h"
#include "CommonSharedConstants.h"
#include "App.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 实现快捷键功能
// 
// Windows 上一般有两种实现热键的方法，它们各有限制: 
// 1. RegisterHotKey: 在某些游戏上不可靠
// 2. 键盘钩子: 如果前台窗口是管理员而 Magpie 不是，此方法无效
// 为了使热键最大程度的可用，这两种方法都被使用。采用下述措施防止它们被同时触发: 
// 1. 键盘钩子会先被触发，然后吞下热键，防止触发 RegisterHotKey
// 2. 限制热键的触发频率
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace ::Magpie;
using namespace winrt::Magpie::implementation;
using namespace winrt;

using winrt::Magpie::ShortcutAction;

namespace Magpie {

void ShortcutService::Initialize() {
	HINSTANCE hInst = wil::GetModuleInstanceHandle();

	WNDCLASSEXW wcex{
		.cbSize = sizeof(wcex),
		.lpfnWndProc = _WndProcStatic,
		.hInstance = hInst,
		.lpszClassName = CommonSharedConstants::HOTKEY_WINDOW_CLASS_NAME
	};
	RegisterClassEx(&wcex);

	_hwndHotkey.reset(CreateWindow(CommonSharedConstants::HOTKEY_WINDOW_CLASS_NAME,
		nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInst, 0));

	for (int i = 0; i < (int)ShortcutAction::COUNT_OR_NONE; ++i) {
		_RegisterShortcut((ShortcutAction)i);
	}

	_shortcutChangedRevoker = AppSettings::Get().ShortcutChanged(
		auto_revoke, std::bind_front(&ShortcutService::_AppSettings_OnShortcutChanged, this));

	_keyboardHook.reset(SetWindowsHookEx(WH_KEYBOARD_LL, _LowLevelKeyboardProc, NULL, NULL));
	if (!_keyboardHook) {
		Logger::Get().Win32Error("SetWindowsHookEx 失败");
	}
}

void ShortcutService::Uninitialize() {
	if (!_hwndHotkey) {
		return;
	}

	_keyboardHook.reset();

	for (int i = 0; i < (int)ShortcutAction::COUNT_OR_NONE; ++i) {
		if (!_shortcutInfos[i].isError) {
			UnregisterHotKey(_hwndHotkey.get(), i);
		}
	}
	
	_hwndHotkey.reset();

	_shortcutChangedRevoker.Revoke();
}

LRESULT ShortcutService::_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_HOTKEY) {
		if (wParam >= 0 && wParam < (UINT)ShortcutAction::COUNT_OR_NONE) {
			ShortcutAction action = (ShortcutAction)wParam;
			Logger::Get().Info(fmt::format("热键 {} 激活（Hotkey）", ShortcutHelper::ToString(action)));
			_FireShortcut(action);
			return 0;
		}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void ShortcutService::_AppSettings_OnShortcutChanged(ShortcutAction action) {
	_RegisterShortcut(action);
}

void ShortcutService::_RegisterShortcut(ShortcutAction action) {
	const Shortcut& shortcut = AppSettings::Get().GetShortcut(action);
	bool& isError = _shortcutInfos[(size_t)action].isError;

	UnregisterHotKey(_hwndHotkey.get(), (int)action);

	if (shortcut.IsEmpty() || ShortcutHelper::CheckShortcut(shortcut) != ShortcutError::NoError) {
		Logger::Get().Win32Error(fmt::format("注册热键 {} 失败", ShortcutHelper::ToString(action)));
		isError = true;
		return;
	}

	UINT modifiers = MOD_NOREPEAT;

	if (shortcut.win) {
		modifiers |= MOD_WIN;
	}
	if (shortcut.ctrl) {
		modifiers |= MOD_CONTROL;
	}
	if (shortcut.alt) {
		modifiers |= MOD_ALT;
	}
	if (shortcut.shift) {
		modifiers |= MOD_SHIFT;
	}

	isError = !RegisterHotKey(_hwndHotkey.get(), (int)action, modifiers, shortcut.code);
	if (isError) {
		Logger::Get().Win32Error(fmt::format("注册热键 {} 失败", ShortcutHelper::ToString(action)));
	}
}

void ShortcutService::_FireShortcut(ShortcutAction action) {
	using namespace std::chrono;

	// 限制触发频率
	auto cur = steady_clock::now();
	auto& lastFireTime = _shortcutInfos[(size_t)action].lastFireTime;
	if (duration_cast<milliseconds>(cur - lastFireTime).count() < 100) {
		return;
	}

	lastFireTime = cur;
	ShortcutActivated.Invoke(action);
}

LRESULT CALLBACK ShortcutService::_LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	ShortcutService& that = Get();
	const KBDLLHOOKSTRUCT* info = ((KBDLLHOOKSTRUCT*)lParam);

	if (nCode < 0 || ((wParam != WM_KEYDOWN) && (wParam != WM_SYSKEYDOWN)) || !that._isKeyboardHookActive) {
		// 遇到为了防止激活开始菜单而发送的假键时不重置 _keyboardHookShortcutActivated
		that._keyboardHookShortcutActivated = info->vkCode == 0xFF && wParam == WM_KEYUP && info->dwExtraInfo == 1;
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	const DWORD code = info->vkCode;
	if (code <= 0 && code >= 255) {
		that._keyboardHookShortcutActivated = false;
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	int codeType = 0;
	switch (code) {
	case VK_LWIN:
	case VK_RWIN:
		codeType = 0;
		break;
	case VK_CONTROL:
	case VK_LCONTROL:
	case VK_RCONTROL:
		codeType = 1;
		break;
	case VK_SHIFT:
	case VK_LSHIFT:
	case VK_RSHIFT:
		codeType = 2;
		break;
	case VK_MENU:
	case VK_LMENU:
	case VK_RMENU:
		codeType = 3;
		break;
	default:
		if (!ShortcutHelper::IsValidKeyCode((uint8_t)code)) {
			that._keyboardHookShortcutActivated = false;
			return CallNextHookEx(NULL, nCode, wParam, lParam);
		}
		codeType = 4;
		break;
	}

	// 获取当前按键状态
	// 在键盘钩子被调用时，GetAsyncKeyState 的状态尚未更新
	Shortcut curKeys;
	curKeys.win = codeType == 0 || (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);
	curKeys.ctrl = codeType == 1 || static_cast<bool>(GetAsyncKeyState(VK_CONTROL) & 0x8000);
	curKeys.shift = codeType == 2 || static_cast<bool>(GetAsyncKeyState(VK_SHIFT) & 0x8000);
	curKeys.alt = codeType == 3 || static_cast<bool>(GetAsyncKeyState(VK_MENU) & 0x8000);
	if (codeType == 4) {
		curKeys.code = (uint8_t)code;
	}

	// 查找匹配热键
	for (uint32_t i = 0; i < (uint32_t)ShortcutAction::COUNT_OR_NONE; ++i) {
		ShortcutAction action = (ShortcutAction)i;
		if (that.IsError(action)) {
			continue;
		}

		if (AppSettings::Get().GetShortcut(action) == curKeys) {
			// 防止长按时重复触发热键
			if (!that._keyboardHookShortcutActivated) {
				that._keyboardHookShortcutActivated = true;

				// 延迟执行回调以缩短钩子的处理时间
				App::Get().Dispatcher().RunAsync(
					CoreDispatcherPriority::Normal,
					[action]() {
						Logger::Get().Info(fmt::format("热键 {} 激活（Keyboard Hook）", ShortcutHelper::ToString(action)));
						Get()._FireShortcut(action);
					}
				);

				if (curKeys.win && !curKeys.ctrl && !curKeys.shift && !curKeys.alt) {
					// 防止激活开始菜单
					INPUT dummyEvent{};
					dummyEvent.type = INPUT_KEYBOARD;
					dummyEvent.ki.wVk = 0xFF;
					dummyEvent.ki.dwFlags = KEYEVENTF_KEYUP;
					dummyEvent.ki.dwExtraInfo = 1;
					SendInput(1, &dummyEvent, sizeof(INPUT));
				}
			}

			// 防止触发由 RegisterHotKey 注册的热键
			return 1;
		}
	}

	that._keyboardHookShortcutActivated = false;
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

}
