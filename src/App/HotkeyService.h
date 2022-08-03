#pragma once
#include <winrt/Magpie.App.h>
#include "WinRTUtils.h"


namespace winrt::Magpie::App {

class HotkeyService {
public:
	static HotkeyService& Get() {
		static HotkeyService instance;
		return instance;
	}

	void Initialize();

	bool IsError(HotkeyAction action) {
		return _errors[(size_t)action];
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

	void OnHotkeyPressed(HotkeyAction action);

private:
	HotkeyService() = default;

	HotkeyService(const HotkeyService&) = delete;
	HotkeyService(HotkeyService&&) = delete;

	~HotkeyService();

	void _Settings_OnHotkeyChanged(HotkeyAction action) {
		_RegisterHotkey(action);
	}

	void _App_OnHwndMainChanged(IInspectable const&, uint64_t value);

	void _RegisterHotkey(HotkeyAction action);

	std::array<bool, (size_t)HotkeyAction::COUNT_OR_NONE> _errors{};

	event<delegate<HotkeyAction>> _hotkeyPressedEvent;

	HWND _hwndMain = NULL;
};

}
