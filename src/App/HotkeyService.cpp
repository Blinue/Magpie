#include "pch.h"
#include "HotkeyService.h"
#include "Logger.h"
#include "HotkeyHelper.h"
#include "AppSettings.h"


namespace winrt::Magpie::App {

HotkeyService::HotkeyService() {
	App app = Application::Current().as<App>();

	_hwndMain = (HWND)app.HwndMain();

	AppSettings::Get().HotkeyChanged({this, &HotkeyService::_Settings_OnHotkeyChanged});

	_RegisterHotkey(HotkeyAction::Scale);
	_RegisterHotkey(HotkeyAction::Overlay);
}

void HotkeyService::OnHotkeyPressed(HotkeyAction action) {
	Logger::Get().Info(fmt::format("热键 {} 激活", HotkeyHelper::ToString(action)));
	_hotkeyPressedEvent(action);
}

HotkeyService::~HotkeyService() {
	for (int i = 0; i < (int)HotkeyAction::COUNT_OR_NONE; ++i) {
		if (!_errors[i]) {
			UnregisterHotKey(_hwndMain, i);
		}
	}
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
		UnregisterHotKey(_hwndMain, (int)action);
	}

	if (!RegisterHotKey(_hwndMain, (int)action, modifiers, hotkey.Code())) {
		Logger::Get().Win32Error(fmt::format("注册热键 {} 失败", HotkeyHelper::ToString(action)));
		_errors[(size_t)action] = true;
	} else {
		_errors[(size_t)action] = false;
	}
}

}
