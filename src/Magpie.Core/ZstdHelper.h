#pragma once

namespace Magpie::Core {

struct ZstdHelper {
	static bool ZstdCompress(std::span<const uint8_t> src, std::vector<uint8_t>& dest, int compressionLevel) noexcept;
	static bool ZstdDecompress(std::span<const uint8_t> src, std::vector<uint8_t>& dest) noexcept;
};

}
