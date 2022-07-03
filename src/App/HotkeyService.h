#pragma once
#include "App.h"


namespace winrt::Magpie::App {

class HotkeyService {
public:
	static HotkeyService& Get() {
		static HotkeyService instance;
		return instance;
	}

	bool IsError(HotkeyAction action) {
		return _errors[(size_t)action];
	}

	event_token HotkeyPressed(delegate<HotkeyAction> const& handler) {
		return _hotkeyPressedEvent.add(handler);
	}

	void HotkeyPressed(event_token const& token) {
		_hotkeyPressedEvent.remove(token);
	}

	void OnHotkeyPressed(HotkeyAction action);

private:
	HotkeyService();

	~HotkeyService();

	void _Settings_OnHotkeyChanged(IInspectable const&, HotkeyAction action) {
		_RegisterHotkey(action);
	}

	void _RegisterHotkey(HotkeyAction action);

	Magpie::App::Settings::HotkeyChanged_revoker _hotkeyChangedRevoker;

	std::array<bool, (size_t)HotkeyAction::COUNT_OR_NONE> _errors;

	event<delegate<HotkeyAction>> _hotkeyPressedEvent;

	Magpie::App::Settings _settings{ nullptr };
	HWND _hwndHost = NULL;
};

}
