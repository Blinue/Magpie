#pragma once

#include "HotkeySettings.g.h"


namespace winrt::Magpie::App::implementation {

struct HotkeySettings : HotkeySettingsT<HotkeySettings> {
	HotkeySettings() = default;

	void CopyFrom(const Magpie::App::HotkeySettings& other);

	bool Equals(const Magpie::App::HotkeySettings& other) const;

	bool IsEmpty() const;

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

	void Code(uint32_t value) noexcept {
		_code = value;
	}

	uint32_t Code() const noexcept {
		return _code;
	}

	IVector<IInspectable> GetKeyList() const;

	HotkeyError Check() const;

	void Clear();

	bool FromString(const hstring& str);
	hstring ToString() const;

private:
	bool _win = false;
	bool _ctrl = false;
	bool _alt = false;
	bool _shift = false;

	// 0 表示无 Virtual Key
	uint32_t _code = 0;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HotkeySettings : HotkeySettingsT<HotkeySettings, implementation::HotkeySettings> {
};

}
