#include "pch.h"
#include "EffectCacheManager.h"
#include <regex>
#include "StrUtils.h"
#include "Logger.h"
#include "CommonSharedConstants.h"
#include <d3dcompiler.h>
#include <zstd.h>
#include "Utils.h"

// YAS 暂不支持 ARM64
// https://github.com/niXman/yas/pull/121
#ifdef _M_ARM64
#define _LITTLE_ENDIAN
#endif
#pragma warning(push)
// C4458：“size”的声明隐藏了类成员
// C4127：条件表达式是常量
#pragma warning(disable: 4458 4127)
#include <yas/mem_streams.hpp>
#include <yas/binary_oarchive.hpp>
#include <yas/binary_iarchive.hpp>
#include <yas/types/std/pair.hpp>
#include <yas/types/std/string.hpp>
#include <yas/types/std/vector.hpp>
#include <yas/types/std/variant.hpp>
#pragma warning(pop)


namespace yas::detail {

// SmallVector
template<std::size_t F, typename T, unsigned N>
struct serializer<
	type_prop::not_a_fundamental,
	ser_case::use_internal_serializer,
	F,
	SmallVector<T, N>
> {
	template<typename Archive>
	static Archive& save(Archive& ar, const SmallVector<T, N>& vector) {
		return concepts::array::save<F>(ar, vector);
	}

	template<typename Archive>
	static Archive& load(Archive& ar, SmallVector<T, N>& vector) {
		return concepts::array::load<F>(ar, vector);
	}
};

// winrt::com_ptr<ID3DBlob>
template<std::size_t F>
struct serializer<
	type_prop::not_a_fundamental,
	ser_case::use_internal_serializer,
	F,
	winrt::com_ptr<ID3DBlob>
> {
	template<typename Archive>
	static Archive& save(Archive& ar, const winrt::com_ptr<ID3DBlob>& blob) {
		uint32_t size = (uint32_t)blob->GetBufferSize();
		ar& size;

		BYTE* buf = (BYTE*)blob->GetBufferPointer();
		for (uint32_t i = 0; i < size; ++i) {
			ar& (*buf++);
		}

		return ar;
	}

	template<typename Archive>
	static Archive& load(Archive& ar, winrt::com_ptr<ID3DBlob>& blob) {
		uint32_t size = 0;
		ar& size;
		HRESULT hr = D3DCreateBlob(size, blob.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("D3DCreateBlob 失败", hr);
			throw new std::exception();
		}

		BYTE* buf = (BYTE*)blob->GetBufferPointer();
		for (uint32_t i = 0; i < size; ++i) {
			ar& (*buf++);
		}

		return ar;
	}
};

}

namespace Magpie::Core {

template<typename Archive, typename T>
void serialize(Archive& ar, EffectConstant<T>& o) {
	ar& o.defaultValue& o.minValue& o.maxValue& o.step;
}

template<typename Archive>
void serialize(Archive& ar, EffectParameterDesc& o) {
	ar& o.name& o.label& o.constant;
}

template<typename Archive>
void serialize(Archive& ar, EffectIntermediateTextureDesc& o) {
	ar& o.format& o.name& o.source& o.sizeExpr;
}

template<typename Archive>
void serialize(Archive& ar, EffectSamplerDesc& o) {
	ar& o.filterType& o.addressType& o.name;
}

template<typename Archive>
void serialize(Archive& ar, EffectPassDesc& o) {
	ar& o.cso& o.inputs& o.outputs& o.numThreads[0] & o.numThreads[1] & o.numThreads[2] & o.blockSize& o.desc& o.isPSStyle;
}

template<typename Archive>
void serialize(Archive& ar, EffectDesc& o) {
	ar& o.name& o.outSizeExpr& o.params& o.textures& o.samplers& o.passes& o.flags;
}

static bool ZstdCompress(std::span<const BYTE> src, std::vector<BYTE>& dest, int compressionLevel) noexcept {
	dest.resize(ZSTD_compressBound(src.size()));
	size_t size = ZSTD_compress(dest.data(), dest.size(), src.data(), src.size(), compressionLevel);

	if (ZSTD_isError(size)) {
		Logger::Get().Error(StrUtils::Concat("压缩失败：", ZSTD_getErrorName(size)));
		return false;
	}

	dest.resize(size);
	return true;
}

static bool ZstdDecompress(std::span<const BYTE> src, std::vector<BYTE>& dest) noexcept {
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

static constexpr const uint32_t MAX_CACHE_COUNT = 127;

// 缓存版本
// 当缓存文件结构有更改时更新它，使旧缓存失效
static constexpr const uint32_t CACHE_VERSION = 10;

// 缓存的压缩等级
static constexpr const int CACHE_COMPRESSION_LEVEL = 1;


static std::wstring GetLinearEffectName(std::wstring_view effectName) {
	std::wstring result(effectName);
	for (wchar_t& c : result) {
		if (c == L'\\') {
			c = L'#';
		}
	}
	return result;
}

static std::wstring GetCacheFileName(std::wstring_view linearEffectName, std::wstring_view hash, UINT flags) {
	// 缓存文件的命名：{效果名}_{标志位（16进制）}{哈希}
	return fmt::format(L"{}{}_{:01x}{}", CommonSharedConstants::CACHE_DIR, linearEffectName, flags & 0xf, hash);
}

void EffectCacheManager::_AddToMemCache(const std::wstring& cacheFileName, const EffectDesc& desc) {
	std::scoped_lock lk(_srwMutex);

	_memCache[cacheFileName] = { desc, ++_lastAccess };

	if (_memCache.size() > MAX_CACHE_COUNT) {
		assert(_memCache.size() == MAX_CACHE_COUNT + 1);

		// 清理一半较旧的内存缓存
		std::array<uint32_t, MAX_CACHE_COUNT + 1> access{};
		std::transform(_memCache.begin(), _memCache.end(), access.begin(),
			[](const auto& pair) {return pair.second.second; });

		auto midIt = access.begin() + access.size() / 2;
		std::nth_element(access.begin(), midIt, access.end());
		const uint32_t mid = *midIt;

		for (auto it = _memCache.begin(); it != _memCache.end();) {
			if (it->second.second < mid) {
				it = _memCache.erase(it);
			} else {
				++it;
			}
		}

		Logger::Get().Info("已清理内存缓存");
	}
}

bool EffectCacheManager::_LoadFromMemCache(const std::wstring& cacheFileName, EffectDesc& desc) {
	std::scoped_lock lk(_srwMutex);

	auto it = _memCache.find(cacheFileName);
	if (it != _memCache.end()) {
		desc = it->second.first;
		it->second.second = ++_lastAccess;
		Logger::Get().Info(StrUtils::Concat("已读取缓存 ", StrUtils::UTF16ToUTF8(cacheFileName)));
		return true;
	}
	return false;
}

bool EffectCacheManager::Load(std::wstring_view effectName, std::wstring_view hash, EffectDesc& desc) {
	assert(!effectName.empty() && !hash.empty());

	std::wstring cacheFileName = GetCacheFileName(GetLinearEffectName(effectName), hash, desc.flags);

	if (_LoadFromMemCache(cacheFileName, desc)) {
		return true;
	}

	if (!Win32Utils::FileExists(cacheFileName.c_str())) {
		return false;
	}

	std::vector<BYTE> buf;
	{
		std::vector<BYTE> compressedBuf;
		if (!Win32Utils::ReadFile(cacheFileName.c_str(), compressedBuf) || compressedBuf.empty()) {
			return false;
		}

		if (!ZstdDecompress(compressedBuf, buf)) {
			Logger::Get().Error("解压缓存失败");
			return false;
		}
	}

	try {
		yas::mem_istream mi(buf.data(), buf.size());
		yas::binary_iarchive<yas::mem_istream, yas::binary> ia(mi);

		ia& desc;
	} catch (...) {
		Logger::Get().Error("反序列化失败");
		desc = {};
		return false;
	}

	_AddToMemCache(cacheFileName, desc);

	Logger::Get().Info(StrUtils::Concat("已读取缓存 ", StrUtils::UTF16ToUTF8(cacheFileName)));
	return true;
}

void EffectCacheManager::Save(std::wstring_view effectName, std::wstring_view hash, const EffectDesc& desc) {
	std::wstring linearEffectName = GetLinearEffectName(effectName);

	std::vector<BYTE> compressedBuf;
	{
		std::vector<BYTE> buf;
		buf.reserve(4096);

		try {
			yas::vector_ostream os(buf);
			yas::binary_oarchive<yas::vector_ostream<BYTE>, yas::binary> oa(os);

			oa& desc;
		} catch (...) {
			Logger::Get().Error("序列化失败");
			return;
		}


		if (!ZstdCompress(buf, compressedBuf, CACHE_COMPRESSION_LEVEL)) {
			Logger::Get().Error("压缩缓存失败");
			return;
		}
	}

	if (!Win32Utils::DirExists(CommonSharedConstants::CACHE_DIR)) {
		if (!CreateDirectory(CommonSharedConstants::CACHE_DIR, nullptr)) {
			Logger::Get().Win32Error("创建 cache 文件夹失败");
			return;
		}
	} else {
		// 删除所有该效果（flags 相同）的缓存
		std::wregex regex(fmt::format(L"^{}_{:01x}[0-9,a-f]{{16}}$", linearEffectName, desc.flags & 0xf),
			std::wregex::optimize | std::wregex::nosubs);

		WIN32_FIND_DATA findData{};
		HANDLE hFind = Win32Utils::SafeHandle(FindFirstFileEx(
			StrUtils::ConcatW(CommonSharedConstants::CACHE_DIR, L"*").c_str(),
			FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
		if (hFind) {
			do {
				// 缓存文件名至少有 19 个字符
				// {Name}_{1}{16}
				if (StrUtils::StrLen(findData.cFileName) < 19) {
					continue;
				}

				// 正则匹配文件名
				if (!std::regex_match(findData.cFileName, regex)) {
					continue;
				}

				if (!DeleteFile(StrUtils::ConcatW(CommonSharedConstants::CACHE_DIR, findData.cFileName).c_str())) {
					Logger::Get().Win32Error(StrUtils::Concat("删除缓存文件 ",
						StrUtils::UTF16ToUTF8(findData.cFileName), " 失败"));
				}
			} while (FindNextFile(hFind, &findData));

			FindClose(hFind);
		} else {
			Logger::Get().Win32Error("查找缓存文件失败");
		}
	}

	std::wstring cacheFileName = GetCacheFileName(linearEffectName, hash, desc.flags);
	if (!Win32Utils::WriteFile(cacheFileName.c_str(), compressedBuf.data(), compressedBuf.size())) {
		Logger::Get().Error("保存缓存失败");
	}

	_AddToMemCache(cacheFileName, desc);

	Logger::Get().Info(StrUtils::Concat("已保存缓存 ", StrUtils::UTF16ToUTF8(cacheFileName)));
}

static std::wstring HexHash(std::span<const BYTE> data) {
	uint64_t hashBytes = Utils::HashData(data);
	
	static wchar_t oct2Hex[16] = {
		L'0',L'1',L'2',L'3',L'4',L'5',L'6',L'7',
		L'8',L'9',L'a',L'b',L'c',L'd',L'e',L'f'
	};

	std::wstring result(16, 0);
	wchar_t* pResult = &result[0];
	
	BYTE* b = (BYTE*)&hashBytes;
	for (int i = 0; i < 8; ++i) {
		*pResult++ = oct2Hex[(*b >> 4) & 0xf];
		*pResult++ = oct2Hex[*b & 0xf];
		++b;
	}

	return result;
}

std::wstring EffectCacheManager::GetHash(
	std::string_view source,
	const phmap::flat_hash_map<std::wstring, float>* inlineParams
) {
	std::string str;
	str.reserve(source.size() + 256);
	str = source;

	str.append(fmt::format("CACHE_VERSION:{}\n", CACHE_VERSION));
	if (inlineParams) {
		for (const auto& pair : *inlineParams) {
			str.append(fmt::format("{}:{}\n", StrUtils::UTF16ToUTF8(pair.first), std::lroundf(pair.second * 10000)));
		}
	}

	return HexHash(std::span((const BYTE*)source.data(), source.size()));
}

std::wstring EffectCacheManager::GetHash(std::string& source, const phmap::flat_hash_map<std::wstring, float>* inlineParams) {
	size_t originSize = source.size();

	source.reserve(originSize + 256);

	source.append(fmt::format("CACHE_VERSION:{}\n", CACHE_VERSION));
	if (inlineParams) {
		for (const auto& pair : *inlineParams) {
			source.append(fmt::format("{}:{}\n", StrUtils::UTF16ToUTF8(pair.first), std::lroundf(pair.second * 10000)));
		}
	}

	std::wstring result = HexHash(std::span((const BYTE*)source.data(), source.size()));
	source.resize(originSize);
	return result;
}

}

#undef _LITTLE_ENDIAN
