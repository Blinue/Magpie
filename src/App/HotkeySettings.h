#pragma once

#include "HotkeySettings.g.h"

namespace winrt::Magpie::implementation {

struct HotkeySettings : HotkeySettingsT<HotkeySettings> {
	HotkeySettings() = default;

	void Win(bool value) noexcept {
		_win = value;
	}
	bool Win() const noexcept {
		return _win;
	}

	void Ctrl(bool value) noexcept {
		_ctrl = value;
	}
	bool Ctrl() const noexcept {
		return _ctrl;
	}

	void Alt(bool value) noexcept {
		_alt = value;
	}
	bool Alt() const noexcept {
		return _alt;
	}

	void Shift(bool value) noexcept {
		_shift = value;
	}
	bool Shift() const noexcept {
		return _shift;
	}

	IVector<IInspectable> GetKeyList() const;

private:
	bool _win = false;
	bool _ctrl = false;
	bool _alt = false;
	bool _shift = false;
};

}

namespace winrt::Magpie::factory_implementation {

struct HotkeySettings : HotkeySettingsT<HotkeySettings, implementation::HotkeySettings> {
};

}
