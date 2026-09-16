#include "pch.h"
#include "Version.h"
#include "StrHelper.h"

namespace Magpie {

bool Version::Parse(std::string_view str) noexcept {
	if (str.empty()) {
		return false;
	}

	SmallVector<std::string_view> numbers = StrHelper::Split(str, '.');
	size_t size = numbers.size();
	if (size != 2 && size != 3) {
		return false;
	}

	const char* end = numbers[0].data() + numbers[0].size();
	auto result = std::from_chars(numbers[0].data(), end, major);
	if (result.ec != std::errc{} || result.ptr != end) {
		return false;
	}

	end = numbers[1].data() + numbers[1].size();
	result = std::from_chars(numbers[1].data(), end, minor);
	if (result.ec != std::errc{} || result.ptr != end) {
		return false;
	}

	if (size == 3) {
		end = numbers[2].data() + numbers[2].size();
		result = std::from_chars(numbers[2].data(), end, patch);
		if (result.ec != std::errc{} || result.ptr != end) {
			return false;
		}
	}

	return true;
}

}
