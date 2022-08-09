#include "pch.h"
#include "Utils.h"
#include "Logger.h"
#include "StrUtils.h"
#include <zstd.h>


bool Utils::ZstdCompress(std::span<const BYTE> src, std::vector<BYTE>& dest, int compressionLevel) {
	dest.resize(ZSTD_compressBound(src.size()));
	size_t size = ZSTD_compress(dest.data(), dest.size(), src.data(), src.size(), compressionLevel);

	if (ZSTD_isError(size)) {
		Logger::Get().Error(StrUtils::Concat("压缩失败：", ZSTD_getErrorName(size)));
		return false;
	}

	dest.resize(size);
	return true;
}

bool Utils::ZstdDecompress(std::span<const BYTE> src, std::vector<BYTE>& dest) {
	auto size = ZSTD_getFrameContentSize(src.data(), src.size());
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
