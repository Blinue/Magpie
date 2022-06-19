#include "pch.h"
#include "HotkeyManager.h"
#if __has_include("HotkeyManager.g.cpp")
#include "HotkeyManager.g.cpp"
#endif
#include "Logger.h"


namespace winrt::Magpie::App::implementation {

HotkeyManager::HotkeyManager() {
	App app = Application::Current().as<App>();
	_settings = app.Settings();
	_hwndHost = (HWND)app.HwndHost();

	_hotkeyChangedToken = _settings.HotkeyChanged(
		auto_revoke, { this,&HotkeyManager::_Settings_OnHotkeyChanged });

	_RegisterHotkey(HotkeyAction::Scale);
	_RegisterHotkey(HotkeyAction::Overlay);
}

bool HotkeyManager::IsError(HotkeyAction action) {
	return _errors[(size_t)action];
}

void HotkeyManager::_Settings_OnHotkeyChanged(IInspectable const&, HotkeyAction action) {
	_RegisterHotkey(action);
}

void HotkeyManager::_RegisterHotkey(HotkeyAction action) {
	HotkeySettings hotkey = _settings.GetHotkey(action);
	if (hotkey == nullptr || hotkey.IsEmpty()) {
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
		UnregisterHotKey(_hwndHost, (int)action);
	}
	
	if (!RegisterHotKey(_hwndHost, (int)action, modifiers, hotkey.Code())) {
		Logger::Get().Win32Error("RegisterHotKey 失败");
		_errors[(size_t)action] = true;
	} else {
		_errors[(size_t)action] = false;
	}
}

}
