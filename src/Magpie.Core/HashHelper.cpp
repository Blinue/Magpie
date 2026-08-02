#include "pch.h"
#include "HashHelper.h"

namespace Magpie {

// FNV-1a. 仅用于缓存键，不需要密码学强度，但必须稳定
// FNV-1a. Only used as a cache key - no cryptographic strength needed, but it
// must stay stable across runs or every engine would rebuild.
uint64_t HashHelper::Hash64(std::span<const uint8_t> data) noexcept {
	uint64_t hash = 14695981039346656037ULL;
	for (uint8_t b : data) {
		hash ^= (uint64_t)b;
		hash *= 1099511628211ULL;
	}
	return hash;
}

std::wstring HashHelper::HexHash(std::span<const uint8_t> data) noexcept {
	const uint64_t hash = Hash64(data);

	std::wstring result(16, L'0');
	for (int i = 15; i >= 0; --i) {
		const uint8_t nibble = (uint8_t)((hash >> ((15 - i) * 4)) & 0xF);
		result[i] = nibble < 10 ? (wchar_t)(L'0' + nibble) : (wchar_t)(L'a' + nibble - 10);
	}
	return result;
}

}
