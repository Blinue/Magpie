#pragma once
#include <winrt/Magpie.UI.h>
#include <variant>
#include "SmallVector.h"


namespace winrt::Magpie::UI {

struct HotkeySettings {
	bool operator==(const HotkeySettings&) const noexcept = default;

	bool IsEmpty() const noexcept;

	SmallVector<std::variant<uint8_t, std::wstring>, 5> GetKeyList() const noexcept;

	HotkeyError Check() const noexcept;

	void Clear() noexcept;

	std::wstring ToString() const noexcept;

	bool win = false;
	bool ctrl = false;
	bool alt = false;
	bool shift = false;

	// 0 表示无 Virtual Key
	uint8_t code = 0;
};

}
