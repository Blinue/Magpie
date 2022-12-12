#pragma once
#include <compare>
#include <tuple>
#include "StrUtils.h"
#include <charconv>

struct Version {
	constexpr Version() {}
	constexpr Version(uint32_t major, uint32_t minor, uint32_t patch)
		: major(major), minor(minor), patch(patch) {}

	std::strong_ordering operator<=>(const Version& other) const noexcept {
		return std::make_tuple(major, minor, patch) <=> std::make_tuple(other.major, other.minor, other.patch);
	}

	bool Parse(std::string_view str) {
		if (str.empty()) {
			return false;
		}

		SmallVector<std::string_view> s;
		SmallVector<std::string_view> numbers = StrUtils::Split(str, '.');
		size_t size = numbers.size();
		if (size > 3) {
			return false;
		}

		assert(size > 0);

		const char* last = numbers[0].data() + numbers[0].size();
		if (std::from_chars(numbers[0].data(), last, major).ptr != last) {
			return false;
		}
		if (size > 1) {
			last = numbers[1].data() + numbers[1].size();
			if (std::from_chars(numbers[1].data(), last, minor).ptr != last) {
				return false;
			}
		}
		if (size > 2) {
			last = numbers[2].data() + numbers[2].size();
			if (std::from_chars(numbers[2].data(), last, patch).ptr != last) {
				return false;
			}
		}

		return true;
	}

	uint32_t major = 0;
	uint32_t minor = 0;
	uint32_t patch = 0;
};

constexpr inline Version MAGPIE_VERSION(0, 9, 100);
constexpr inline const char* MAGPIE_TAG = "v0.10.0-preview1";
constexpr inline const wchar_t* MAGPIE_TAG_W = L"v0.10.0-preview1";
