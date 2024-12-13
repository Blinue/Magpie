#pragma once

namespace Magpie::Core {

struct FastHasher {
	static uint64_t HashData(std::span<const BYTE> data) noexcept;
};

}
