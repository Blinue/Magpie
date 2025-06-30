#pragma once
#include <compare>
#include <tuple>
#include <fmt/format.h>

struct Version {
	constexpr Version() {}
	constexpr Version(uint32_t major, uint32_t minor, uint32_t patch)
		: major(major), minor(minor), patch(patch) {}

	// 默认逐成员比较
	std::strong_ordering operator<=>(const Version&) const = default;

	bool Parse(std::string_view str) noexcept;

	template<typename CHAR_T>
	std::basic_string<CHAR_T> ToString() const noexcept {
		if constexpr (std::is_same_v<CHAR_T, char>) {
			return fmt::format("{}.{}.{}", major, minor, patch);
		} else {
			return fmt::format(L"{}.{}.{}", major, minor, patch);
		}
	}

	uint32_t major = 0;
	uint32_t minor = 0;
	uint32_t patch = 0;
};
