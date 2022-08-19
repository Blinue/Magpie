#pragma once
#include <winrt/Magpie.App.h>
#include <variant>


namespace winrt::Magpie::App {

struct HotkeySettings {
	bool operator==(const HotkeySettings&) const noexcept = default;

	bool IsEmpty() const noexcept;

	std::vector<std::variant<uint32_t, std::wstring>> GetKeyList() const noexcept;

	HotkeyError Check() const noexcept;

	void Clear() noexcept;

	bool FromString(std::wstring_view str) noexcept;
	std::wstring ToString() const noexcept;

	bool win = false;
	bool ctrl = false;
	bool alt = false;
	bool shift = false;

	// 0 表示无 Virtual Key
	uint32_t code = 0;
};

}
