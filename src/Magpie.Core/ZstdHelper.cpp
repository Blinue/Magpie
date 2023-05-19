#include "pch.h"
#include "ZstdHelper.h"
#include <zstd.h>
#include <span>
#include "StrUtils.h"
#include "Logger.h"

namespace Magpie::Core {

bool ZstdHelper::ZstdCompress(std::span<const uint8_t> src, std::vector<uint8_t>& dest, int compressionLevel) noexcept {
	dest.resize(ZSTD_compressBound(src.size()));
	size_t size = ZSTD_compress(dest.data(), dest.size(), src.data(), src.size(), compressionLevel);

	if (ZSTD_isError(size)) {
		Logger::Get().Error(StrUtils::Concat("压缩失败：", ZSTD_getErrorName(size)));
		return false;
	}

	dest.resize(size);
	return true;
}

bool ZstdHelper::ZstdDecompress(std::span<const uint8_t> src, std::vector<uint8_t>& dest) noexcept {
	size_t size = ZSTD_getFrameContentSize(src.data(), src.size());
	if (size == ZSTD_CONTENTSIZE_UNKNOWN || size == ZSTD_CONTENTSIZE_ERROR) {
		Logger::Get().Error("ZSTD_getFrameContentSize 失败");
		return false;
	}

	dest.resize(size);
	size = ZSTD_decompress(dest.data(), dest.size(), src.data(), src.size());
	if (ZSTD_isError(size)) {
		Logger::Get().Error(StrUtils::Concat("解压失败：", ZSTD_getErrorName(size)));
		return false;
	}

	dest.resize(size);

	return true;
}

}
