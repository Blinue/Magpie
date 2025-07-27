#pragma once

namespace Magpie {

struct Shortcut {
	bool operator==(const Shortcut&) const noexcept = default;

	bool IsEmpty() const noexcept;

	void Clear() noexcept;

	std::wstring ToString() const noexcept;

	// 0 表示无 Virtual Key
	uint8_t code = 0;

	bool win = false;
	bool ctrl = false;
	bool alt = false;
	bool shift = false;
};

}
