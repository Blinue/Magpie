#include "pch.h"
#include "HotkeyService.h"
#include "Logger.h"
#include "HotkeyHelper.h"
#include "AppSettings.h"
#include "CommonSharedConstants.h"


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

	std::fill(_errors.begin(), _errors.end(), true);
	_RegisterHotkey(HotkeyAction::Scale);
	_RegisterHotkey(HotkeyAction::Overlay);

	AppSettings::Get().HotkeyChanged({ this, &HotkeyService::_Settings_OnHotkeyChanged });
}

HotkeyService::~HotkeyService() {
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
			Logger::Get().Info(fmt::format("热键 {} 激活", HotkeyHelper::ToString(action)));
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

	if (hotkey.Win()) {
		modifiers |= MOD_WIN;
	}
	if (hotkey.Ctrl()) {
		modifiers |= MOD_CONTROL;
	}
	if (hotkey.Alt()) {
		modifiers |= MOD_ALT;
	}
	if (hotkey.Shift()) {
		modifiers |= MOD_SHIFT;
	}

	if (!_errors[(size_t)action]) {
		UnregisterHotKey(_hwndHotkey, (int)action);
	}

	if (!RegisterHotKey(_hwndHotkey, (int)action, modifiers, hotkey.Code())) {
		Logger::Get().Win32Error(fmt::format("注册热键 {} 失败", HotkeyHelper::ToString(action)));
		_errors[(size_t)action] = true;
	} else {
		_errors[(size_t)action] = false;
	}
}

}
