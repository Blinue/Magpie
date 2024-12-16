#pragma once

namespace Magpie {

struct FastHasher {
	static uint64_t HashData(std::span<const BYTE> data) noexcept;
};

}
