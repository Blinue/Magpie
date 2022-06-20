#pragma once

#include "HotkeyManager.g.h"


namespace winrt::Magpie::App::implementation {

struct HotkeyManager : HotkeyManagerT<HotkeyManager> {
	HotkeyManager(Magpie::App::Settings settings, uint64_t hwndHost);
	~HotkeyManager();

	bool IsError(HotkeyAction action);

	event_token HotkeyPressed(EventHandler<HotkeyAction> const& handler);
	void HotkeyPressed(event_token const& token);

	void OnHotkeyPressed(HotkeyAction action);

private:
	void _Settings_OnHotkeyChanged(IInspectable const&, HotkeyAction action);

	void _RegisterHotkey(HotkeyAction action);

	Magpie::App::Settings::HotkeyChanged_revoker _hotkeyChangedRevoker;

	std::array<bool, (size_t)HotkeyAction::COUNT_OR_NONE> _errors;

	event<EventHandler<HotkeyAction>> _hotkeyPressedEvent;

	Magpie::App::Settings _settings{ nullptr };
	HWND _hwndHost = NULL;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HotkeyManager : HotkeyManagerT<HotkeyManager, implementation::HotkeyManager> {
};

}
