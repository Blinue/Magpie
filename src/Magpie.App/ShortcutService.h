#pragma once
#include <winrt/Magpie.App.h>
#include "WinRTUtils.h"

namespace winrt::Magpie::App {

class ShortcutService {
public:
	static ShortcutService& Get() noexcept {
		static ShortcutService instance;
		return instance;
	}

	ShortcutService(const ShortcutService&) = delete;
	ShortcutService(ShortcutService&&) = delete;

	void Initialize();

	void Uninitialize();

	bool IsError(ShortcutAction action) const noexcept {
		return _ShortcutInfos[(size_t)action].isError;
	}

	event_token HotkeyPressed(delegate<ShortcutAction> const& handler) {
		return _hotkeyPressedEvent.add(handler);
	}

	WinRTUtils::EventRevoker HotkeyPressed(auto_revoke_t, delegate<ShortcutAction> const& handler) {
		event_token token = HotkeyPressed(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			HotkeyPressed(token);
		});
	}

	void HotkeyPressed(event_token const& token) {
		_hotkeyPressedEvent.remove(token);
	}

	void StopKeyboardHook() noexcept {
		_isKeyboardHookActive = false;
	}

	void StartKeyboardHook() noexcept {
		_isKeyboardHookActive = true;
	}

private:
	ShortcutService() = default;
	~ShortcutService();

	static LRESULT _WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return Get()._WndProc(hWnd, msg, wParam, lParam);
	}
	LRESULT _WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void _AppSettings_OnHotkeyChanged(ShortcutAction action);

	void _RegisterShortcut(ShortcutAction action);

	void _FireShortcut(ShortcutAction action);

	static LRESULT CALLBACK _LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

	struct _ShortcutInfo {
		std::chrono::steady_clock::time_point lastFireTime{};
		bool isError = true;
	};
	std::array<_ShortcutInfo, (size_t)ShortcutAction::COUNT_OR_NONE> _ShortcutInfos{};
	event<delegate<ShortcutAction>> _hotkeyPressedEvent;
	HWND _hwndHotkey = NULL;
	HHOOK _keyboardHook = NULL;

	bool _isKeyboardHookActive = true;
	// 用于防止长按时重复触发热键
	bool _keyboardHookHotkeyFired = false;
};

}
