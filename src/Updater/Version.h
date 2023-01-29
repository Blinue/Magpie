#pragma once
#include "pch.h"
#include <charconv>

struct Version {
	constexpr Version() {}
	constexpr Version(uint32_t major, uint32_t minor, uint32_t patch)
		: major(major), minor(minor), patch(patch) {
	}

	std::strong_ordering operator<=>(const Version& other) const noexcept {
		return std::make_tuple(major, minor, patch) <=> std::make_tuple(other.major, other.minor, other.patch);
	}

	bool Parse(std::string_view str) {
		if (str.empty()) {
			return false;
		}

		std::vector<std::string_view> numbers;
		// 分割
		do {
			size_t pos = str.find(L'.', 0);
			numbers.push_back(str.substr(0, pos));

			if (pos == std::string_view::npos) {
				break;
			}

			str.remove_prefix(pos + 1);
		} while (!str.empty());

		size_t size = numbers.size();
		if (size != 2 && size != 3) {
			return false;
		}

		const char* last = numbers[0].data() + numbers[0].size();
		if (std::from_chars(numbers[0].data(), last, major).ptr != last) {
			return false;
		}

		last = numbers[1].data() + numbers[1].size();
		if (std::from_chars(numbers[1].data(), last, minor).ptr != last) {
			return false;
		}

		if (size == 3) {
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
