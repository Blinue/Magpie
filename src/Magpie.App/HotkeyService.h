#pragma once
#include <winrt/Magpie.App.h>
#include "WinRTUtils.h"

namespace winrt::Magpie::App {

class HotkeyService {
public:
	static HotkeyService& Get() noexcept {
		static HotkeyService instance;
		return instance;
	}

	HotkeyService(const HotkeyService&) = delete;
	HotkeyService(HotkeyService&&) = delete;

	void Initialize();

	void Destory();

	bool IsError(HotkeyAction action) const noexcept {
		return _hotkeyInfos[(size_t)action].isError;
	}

	event_token HotkeyPressed(delegate<HotkeyAction> const& handler) {
		return _hotkeyPressedEvent.add(handler);
	}

	WinRTUtils::EventRevoker HotkeyPressed(auto_revoke_t, delegate<HotkeyAction> const& handler) {
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
	HotkeyService() = default;
	~HotkeyService();

	static LRESULT _WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return Get()._WndProc(hWnd, msg, wParam, lParam);
	}
	LRESULT _WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void _Settings_OnHotkeyChanged(HotkeyAction action);

	void _RegisterHotkey(HotkeyAction action);

	void _FireHotkey(HotkeyAction action);

	static LRESULT CALLBACK _LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

	struct _HotkeyInfo {
		std::chrono::steady_clock::time_point lastFireTime{};
		bool isError = true;
	};
	std::array<_HotkeyInfo, (size_t)HotkeyAction::COUNT_OR_NONE> _hotkeyInfos{};
	event<delegate<HotkeyAction>> _hotkeyPressedEvent;
	HWND _hwndHotkey = NULL;
	HHOOK _keyboardHook = NULL;

	bool _isKeyboardHookActive = true;
	// 用于防止长按时重复触发热键
	bool _keyboardHookHotkeyFired = false;
};

}
