#pragma once
#include <winrt/Magpie.App.h>
#include <variant>


namespace winrt::Magpie::App {

class HotkeySettings {
public:
	HotkeySettings() noexcept = default;
	HotkeySettings(const HotkeySettings&) noexcept = default;
	HotkeySettings(HotkeySettings&&) noexcept = default;

	HotkeySettings& operator=(const HotkeySettings&) noexcept = default;
	HotkeySettings& operator=(HotkeySettings&&) noexcept = default;

	bool operator==(const HotkeySettings&) const noexcept = default;

	bool IsEmpty() const noexcept;

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

	std::vector<std::variant<uint32_t, std::wstring>> GetKeyList() const noexcept;

	HotkeyError Check() const noexcept;

	void Clear() noexcept;

	bool FromString(std::wstring_view str) noexcept;
	std::wstring ToString() const noexcept;

private:
	bool _win = false;
	bool _ctrl = false;
	bool _alt = false;
	bool _shift = false;

	// 0 表示无 Virtual Key
	uint32_t _code = 0;
};

}
