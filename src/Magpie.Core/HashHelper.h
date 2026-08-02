#pragma once

namespace Magpie {

// 上游移除了 Utils::HashData，这里自带实现以免依赖已删除的头文件
// Upstream removed Utils::HashData, so this carries its own implementation
// rather than depending on a header that no longer exists.
struct HashHelper {
	static uint64_t Hash64(std::span<const uint8_t> data) noexcept;

	static std::wstring HexHash(std::span<const uint8_t> data) noexcept;
};

}
