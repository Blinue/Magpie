#include "pch.h"
#include "HotkeyService.h"
#include "Logger.h"
#include "HotkeyHelper.h"
#include "AppSettings.h"
#include "CommonSharedConstants.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 实现快捷键功能
// 
// Windows 上一般有两种实现热键的方法，它们各有限制：
// 1. RegisterHotKey：在某些游戏上不可靠
// 2. 键盘钩子：如果前台窗口是管理员而 Magpie 不是，此方法无效
// 为了使热键最大程度的可用，这两种方法都被使用。采用下述措施防止它们被同时触发：
// 1. 键盘钩子会先被触发，然后吞下热键，防止触发 RegisterHotKey
// 2. 限制热键的触发频率
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


namespace winrt::Magpie::App {

void HotkeyService::Initialize() {
	HINSTANCE hInst = GetModuleHandle(nullptr);

	WNDCLASSEXW wcex{};
	wcex.cbSize = sizeof(wcex);
	wcex.hInstance = hInst;
	wcex.lpfnWndProc = _WndProcStatic;
	wcex.lpszClassName = CommonSharedConstants::HOTKEY_WINDOW_CLASS_NAME;
	RegisterClassEx(&wcex);

	_hwndHotkey = CreateWindow(CommonSharedConstants::HOTKEY_WINDOW_CLASS_NAME, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInst, 0);

	_RegisterHotkey(HotkeyAction::Scale);
	_RegisterHotkey(HotkeyAction::Overlay);

	AppSettings::Get().HotkeyChanged({ this, &HotkeyService::_Settings_OnHotkeyChanged });

	_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, _LowLevelKeyboardProc, NULL, NULL);
	if (!_keyboardHook) {
		Logger::Get().Win32Error("SetWindowsHookEx 失败");
	}
}

void HotkeyService::Destory() {
	if (!_hwndHotkey) {
		return;
	}

	if (_keyboardHook) {
		UnhookWindowsHookEx(_keyboardHook);
	}

	for (int i = 0; i < (int)HotkeyAction::COUNT_OR_NONE; ++i) {
		if (!_hotkeyInfos[i].isError) {
			UnregisterHotKey(_hwndHotkey, i);
		}
	}

	DestroyWindow(_hwndHotkey);
	_hwndHotkey = NULL;
}

HotkeyService::~HotkeyService() {
	Destory();
}

LRESULT HotkeyService::_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_HOTKEY) {
		if (wParam >= 0 && wParam < (UINT)HotkeyAction::COUNT_OR_NONE) {
			HotkeyAction action = (HotkeyAction)wParam;
			Logger::Get().Info(fmt::format("热键 {} 激活（Hotkey）", HotkeyHelper::ToString(action)));
			_FireHotkey(action);
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
	bool& isError = _hotkeyInfos[(size_t)action].isError;

	UnregisterHotKey(_hwndHotkey, (int)action);

	if (hotkey.IsEmpty() || hotkey.Check() != HotkeyError::NoError) {
		Logger::Get().Win32Error(fmt::format("注册热键 {} 失败", HotkeyHelper::ToString(action)));
		isError = true;
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

	isError = !RegisterHotKey(_hwndHotkey, (int)action, modifiers, hotkey.code);
	if (isError) {
		Logger::Get().Win32Error(fmt::format("注册热键 {} 失败", HotkeyHelper::ToString(action)));
	}
}

void HotkeyService::_FireHotkey(HotkeyAction action) {
	using namespace std::chrono;

	// 限制触发频率
	auto cur = steady_clock::now();
	auto& lastFireTime = _hotkeyInfos[(size_t)action].lastFireTime;
	if (duration_cast<milliseconds>(cur - lastFireTime).count() < 100) {
		return;
	}

	lastFireTime = cur;
	_hotkeyPressedEvent(action);
}

LRESULT CALLBACK HotkeyService::_LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	HotkeyService& that = Get();
	const KBDLLHOOKSTRUCT* info = ((KBDLLHOOKSTRUCT*)lParam);

	if (nCode < 0 || ((wParam != WM_KEYDOWN) && (wParam != WM_SYSKEYDOWN)) || !that._isKeyboardHookActive) {
		// 遇到为了防止激活开始菜单而发送的假键时不重置 _keyboardHookHotkeyFired
		that._keyboardHookHotkeyFired = info->vkCode == 0xFF && wParam == WM_KEYUP && info->dwExtraInfo == 1;
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	const DWORD code = info->vkCode;
	if (code <= 0 && code >= 255) {
		that._keyboardHookHotkeyFired = false;
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
			that._keyboardHookHotkeyFired = false;
			return CallNextHookEx(NULL, nCode, wParam, lParam);
		}
		codeType = 4;
		break;
	}

	// 获取当前按键状态
	// 在键盘钩子被调用时，GetAsyncKeyState 的状态尚未更新
	HotkeySettings curKeys;
	curKeys.win = codeType == 0 || (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);
	curKeys.ctrl = codeType == 1 || static_cast<bool>(GetAsyncKeyState(VK_CONTROL) & 0x8000);
	curKeys.shift = codeType == 2 || static_cast<bool>(GetAsyncKeyState(VK_SHIFT) & 0x8000);
	curKeys.alt = codeType == 3 || static_cast<bool>(GetAsyncKeyState(VK_MENU) & 0x8000);
	if (codeType == 4) {
		curKeys.code = (uint8_t)code;
	}

	// 查找匹配热键
	for (uint32_t i = 0; i < (uint32_t)HotkeyAction::COUNT_OR_NONE; ++i) {
		HotkeyAction action = (HotkeyAction)i;
		if (that.IsError(action)) {
			continue;
		}

		if (AppSettings::Get().GetHotkey(action) == curKeys) {
			// 防止长按时重复触发热键
			if (!that._keyboardHookHotkeyFired) {
				that._keyboardHookHotkeyFired = true;

				// 延迟执行回调以缩短钩子的处理时间
				[](HotkeyAction action) -> fire_and_forget {
					co_await CoreWindow::GetForCurrentThread().Dispatcher().TryRunAsync(
						CoreDispatcherPriority::Normal,
						[action]() {
							Logger::Get().Info(fmt::format("热键 {} 激活（Keyboard Hook）", HotkeyHelper::ToString(action)));
							Get()._FireHotkey(action);
						}
					);
				}(action);

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

	that._keyboardHookHotkeyFired = false;
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

}
