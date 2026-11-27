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

	uint32_t localMajor = 0;
	uint32_t localMinor = 0;
	uint32_t localPatch = 0;

	const char* end = numbers[0].data() + numbers[0].size();
	auto result = std::from_chars(numbers[0].data(), end, localMajor);
	if (result.ec != std::errc{} || result.ptr != end) {
		return false;
	}

	end = numbers[1].data() + numbers[1].size();
	result = std::from_chars(numbers[1].data(), end, localMinor);
	if (result.ec != std::errc{} || result.ptr != end) {
		return false;
	}

	if (size == 3) {
		end = numbers[2].data() + numbers[2].size();
		result = std::from_chars(numbers[2].data(), end, localPatch);
		if (result.ec != std::errc{} || result.ptr != end) {
			return false;
		}
	}

	major = localMajor;
	minor = localMinor;
	patch = localPatch;
	return true;
}

}
