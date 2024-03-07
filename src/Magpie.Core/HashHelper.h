#pragma once

namespace Magpie::Core {

struct HashHelper {
	static std::wstring HexHash(std::span<const uint8_t> data) noexcept;
};

}
