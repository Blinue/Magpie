#include "pch.h"
#include "HotkeyService.h"
#include "Logger.h"
#include "HotkeyHelper.h"
#include "AppSettings.h"


namespace winrt::Magpie::App {

void HotkeyService::Initialize() {
	App app = Application::Current().as<App>();

	app.HwndMainChanged({ this, &HotkeyService::_App_OnHwndMainChanged });
	_App_OnHwndMainChanged(nullptr, app.HwndMain());

	AppSettings::Get().HotkeyChanged({ this, &HotkeyService::_Settings_OnHotkeyChanged });
}

void HotkeyService::OnHotkeyPressed(HotkeyAction action) {
	Logger::Get().Info(fmt::format("热键 {} 激活", HotkeyHelper::ToString(action)));
	_hotkeyPressedEvent(action);
}

HotkeyService::~HotkeyService() {
	if (!_hwndMain) {
		return;
	}

	for (int i = 0; i < (int)HotkeyAction::COUNT_OR_NONE; ++i) {
		if (!_errors[i]) {
			UnregisterHotKey(_hwndMain, i);
		}
	}
}

void HotkeyService::_App_OnHwndMainChanged(IInspectable const&, uint64_t value) {
	_hwndMain = (HWND)value;
	std::fill(_errors.begin(), _errors.end(), false);
	if (value == 0) {
		return;
	}

	_RegisterHotkey(HotkeyAction::Scale);
	_RegisterHotkey(HotkeyAction::Overlay);
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
