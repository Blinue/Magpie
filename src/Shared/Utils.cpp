#include "pch.h"
#include "Utils.h"
#include "Logger.h"
#include "StrUtils.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 哈希算法来自 https://github.com/wangyi-fudan/wyhash/blob/b8b740844c2e9830fd205302df76dcdd4fadcec9/wyhash.h
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////

// multiply and xor mix function, aka MUM
static uint64_t _wymix(uint64_t lhs, uint64_t rhs) noexcept {
#ifdef _M_X64
	uint64_t hi;
	uint64_t lo = _umul128(lhs, rhs, &hi);
#elif defined(_M_ARM64)
	uint64_t lo = lhs * rhs;
	uint64_t hi = __umulh(lhs, rhs);
#else
#error "不支持的 CPU 架构"
#endif
	return lo ^ hi;
}

// read functions
static uint64_t _wyr8(const uint8_t* p) noexcept {
	uint64_t v;
	memcpy(&v, p, 8);
	return v;
}

static uint64_t _wyr4(const uint8_t* p) noexcept {
	uint32_t v;
	memcpy(&v, p, 4);
	return v;
}

static uint64_t _wyr3(const uint8_t* p, size_t k) noexcept {
	return (((uint64_t)p[0]) << 16) | (((uint64_t)p[k >> 1]) << 8) | p[k - 1];
}

// the default secret parameters
static const uint64_t _wyp[4] = { 0xa0761d6478bd642full, 0xe7037ed1a0b428dbull, 0x8ebc6af09c88c6e3ull, 0x589965cc75374cc3ull };

uint64_t Utils::HashData(std::span<const BYTE> data) noexcept {
	const size_t len = data.size();
	uint64_t seed = _wyp[0];

	const uint8_t* p = (const uint8_t*)data.data();
	uint64_t a, b;
	if (len <= 16) {
		if (len >= 4) {
			a = (_wyr4(p) << 32) | _wyr4(p + ((len >> 3) << 2));
			b = (_wyr4(p + len - 4) << 32) | _wyr4(p + len - 4 - ((len >> 3) << 2));
		} else if (len > 0) {
			a = _wyr3(p, len);
			b = 0;
		} else {
			a = b = 0;
		}
	} else {
		size_t i = len;
		if (i > 48) {
			uint64_t see1 = seed, see2 = seed;
			do {
				seed = _wymix(_wyr8(p) ^ _wyp[1], _wyr8(p + 8) ^ seed);
				see1 = _wymix(_wyr8(p + 16) ^ _wyp[2], _wyr8(p + 24) ^ see1);
				see2 = _wymix(_wyr8(p + 32) ^ _wyp[3], _wyr8(p + 40) ^ see2);
				p += 48;
				i -= 48;
			} while (i > 48);
			seed ^= see1 ^ see2;
		}

		while (i > 16) {
			seed = _wymix(_wyr8(p) ^ _wyp[1], _wyr8(p + 8) ^ seed);
			i -= 16;
			p += 16;
		}
		a = _wyr8(p + i - 16);  b = _wyr8(p + i - 8);
	}

	return _wymix(_wyp[1] ^ len, _wymix(a ^ _wyp[1], b ^ seed));
}
