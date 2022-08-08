#pragma once
#include <winrt/Magpie.App.h>
#include <variant>


namespace winrt::Magpie::App {

struct HotkeySettings {
	HotkeySettings() noexcept = default;
	HotkeySettings(const HotkeySettings&) noexcept = default;
	HotkeySettings(HotkeySettings&&) noexcept = default;

	HotkeySettings& operator=(const HotkeySettings&) noexcept = default;
	HotkeySettings& operator=(HotkeySettings&&) noexcept = default;

	bool operator==(const HotkeySettings&) const noexcept = default;

	bool IsEmpty() const noexcept;

	std::vector<std::variant<uint32_t, std::wstring>> GetKeyList() const noexcept;

	HotkeyError Check() const noexcept;

	void Clear() noexcept;

	bool FromString(std::wstring_view str) noexcept;
	std::wstring ToString() const noexcept;

	bool Win = false;
	bool Ctrl = false;
	bool Alt = false;
	bool Shift = false;

	// 0 表示无 Virtual Key
	uint32_t Code = 0;
};

}
