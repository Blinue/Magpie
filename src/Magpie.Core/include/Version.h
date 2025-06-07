#pragma once
#include <compare>
#include <tuple>

namespace Magpie {

struct Version {
	constexpr Version() {}
	constexpr Version(uint32_t major, uint32_t minor, uint32_t patch)
		: major(major), minor(minor), patch(patch) {}

	std::strong_ordering operator<=>(const Version& other) const noexcept {
		return std::make_tuple(major, minor, patch) <=> std::make_tuple(other.major, other.minor, other.patch);
	}

	bool Parse(std::string_view str);

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

}
