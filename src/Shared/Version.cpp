#include "pch.h"
#include "Version.h"
#include "StrUtils.h"

bool Version::Parse(std::string_view str) {
	if (str.empty()) {
		return false;
	}

	SmallVector<std::string_view> numbers = StrUtils::Split(str, '.');
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
