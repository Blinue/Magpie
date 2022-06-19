#pragma once

#include "HotkeyManager.g.h"


namespace winrt::Magpie::App::implementation {

struct HotkeyManager : HotkeyManagerT<HotkeyManager> {
	HotkeyManager();
	~HotkeyManager();

	bool IsError(HotkeyAction action);

private:
	void _Settings_OnHotkeyChanged(IInspectable const&, HotkeyAction action);

	void _RegisterHotkey(HotkeyAction action);

	Magpie::App::Settings::HotkeyChanged_revoker _hotkeyChangedToken;

	std::array<bool, (size_t)HotkeyAction::COUNT_OR_NONE> _errors;

	Magpie::App::Settings _settings{ nullptr };
	HWND _hwndHost = NULL;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HotkeyManager : HotkeyManagerT<HotkeyManager, implementation::HotkeyManager> {
};

}
