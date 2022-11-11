#include "pch.h"
#include "HotkeyService.h"
#include "Logger.h"
#include "HotkeyHelper.h"
#include "AppSettings.h"
#include "CommonSharedConstants.h"


namespace winrt::Magpie::UI {

void HotkeyService::Initialize() {
	HINSTANCE hInst = GetModuleHandle(nullptr);

	WNDCLASSEXW wcex{};
	wcex.cbSize = sizeof(wcex);
	wcex.hInstance = hInst;
	wcex.lpfnWndProc = _WndProcStatic;
	wcex.lpszClassName = CommonSharedConstants::HOTKEY_WINDOW_CLASS_NAME;
	RegisterClassEx(&wcex);

	_hwndHotkey = CreateWindow(CommonSharedConstants::HOTKEY_WINDOW_CLASS_NAME, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInst, 0);

	std::fill(_errors.begin(), _errors.end(), true);
	_RegisterHotkey(HotkeyAction::Scale);
	_RegisterHotkey(HotkeyAction::Overlay);

	AppSettings::Get().HotkeyChanged({ this, &HotkeyService::_Settings_OnHotkeyChanged });

	_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, _LowLevelKeyboardProc, NULL, NULL);
	if (!_keyboardHook) {
		Logger::Get().Win32Error("SetWindowsHookEx 失败");
	}
}

HotkeyService::~HotkeyService() {
	if (_keyboardHook) {
		UnhookWindowsHookEx(_keyboardHook);
	}

	for (int i = 0; i < (int)HotkeyAction::COUNT_OR_NONE; ++i) {
		if (!_errors[i]) {
			UnregisterHotKey(_hwndHotkey, i);
		}
	}

	DestroyWindow(_hwndHotkey);
}

LRESULT HotkeyService::_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_HOTKEY) {
		if (wParam >= 0 && wParam < (UINT)HotkeyAction::COUNT_OR_NONE) {
			HotkeyAction action = (HotkeyAction)wParam;
			Logger::Get().Info(fmt::format("热键 {} 激活（Hotkey）", HotkeyHelper::ToString(action)));
			_hotkeyPressedEvent(action);

			return 0;
		}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void HotkeyService::_Settings_OnHotkeyChanged(HotkeyAction action) {
	_RegisterHotkey(action);
}

void HotkeyService::_RegisterHotkey(HotkeyAction action) {
	const HotkeySettings& hotkey = AppSettings::Get().GetHotkey(action);
	if (hotkey.IsEmpty() || hotkey.Check() != HotkeyError::NoError) {
		Logger::Get().Win32Error(fmt::format("注册热键 {} 失败", HotkeyHelper::ToString(action)));
		_errors[(size_t)action] = true;
		return;
	}

	UINT modifiers = MOD_NOREPEAT;

	if (hotkey.win) {
		modifiers |= MOD_WIN;
	}
	if (hotkey.ctrl) {
		modifiers |= MOD_CONTROL;
	}
	if (hotkey.alt) {
		modifiers |= MOD_ALT;
	}
	if (hotkey.shift) {
		modifiers |= MOD_SHIFT;
	}

	if (!_errors[(size_t)action]) {
		UnregisterHotKey(_hwndHotkey, (int)action);
	}

	if (!RegisterHotKey(_hwndHotkey, (int)action, modifiers, hotkey.code)) {
		Logger::Get().Win32Error(fmt::format("注册热键 {} 失败", HotkeyHelper::ToString(action)));
		_errors[(size_t)action] = true;
	} else {
		_errors[(size_t)action] = false;
	}
}

LRESULT HotkeyService::_LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode < 0 || ((wParam != WM_KEYDOWN) && (wParam != WM_SYSKEYDOWN)) || !Get()._isKeyboardHookActive) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	const DWORD code = ((KBDLLHOOKSTRUCT*)lParam)->vkCode;
	if (code <= 0 && code >= 255) {
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
		if (!HotkeyHelper::IsValidKeyCode((uint8_t)code)) {
			return CallNextHookEx(NULL, nCode, wParam, lParam);
		}
		codeType = 4;
		break;
	}

	// 获取当前按键状态
	HotkeySettings curKeys;
	curKeys.win = codeType == 0 || (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);
	curKeys.ctrl = codeType == 1 || static_cast<bool>(GetAsyncKeyState(VK_CONTROL) & 0x8000);
	curKeys.shift = codeType == 2 || static_cast<bool>(GetAsyncKeyState(VK_SHIFT) & 0x8000);
	curKeys.alt = codeType == 3 || static_cast<bool>(GetAsyncKeyState(VK_MENU) & 0x8000);
	if (codeType == 4) {
		curKeys.code = (uint8_t)code;
	}

	for (uint32_t i = 0; i < (uint32_t)HotkeyAction::COUNT_OR_NONE; ++i) {
		HotkeyAction action = (HotkeyAction)i;
		if (Get().IsError(action)) {
			continue;
		}

		if (AppSettings::Get().GetHotkey(action) == curKeys) {
			[](HotkeyAction action) -> fire_and_forget {
				co_await CoreWindow::GetForCurrentThread().Dispatcher().TryRunAsync(
					CoreDispatcherPriority::Normal,
					[action]() {
						Logger::Get().Info(fmt::format("热键 {} 激活（Keyboard Hook）", HotkeyHelper::ToString(action)));
						Get()._hotkeyPressedEvent(action);
					}
				);
			}(action);

			if (curKeys.win && !curKeys.ctrl && !curKeys.shift && !curKeys.alt) {
				// 防止激活开始菜单
				INPUT dummyEvent{};
				dummyEvent.type = INPUT_KEYBOARD;
				dummyEvent.ki.wVk = 0xFF;
				dummyEvent.ki.dwFlags = KEYEVENTF_KEYUP;
				SendInput(1, &dummyEvent, sizeof(INPUT));
			}
			
			// 防止触发由 RegisterHotKey 注册的热键
			return 1;
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

}
