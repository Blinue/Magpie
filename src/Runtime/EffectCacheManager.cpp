#include "pch.h"
#include "EffectCacheManager.h"
#include <yas/mem_streams.hpp>
#include <yas/binary_oarchive.hpp>
#include <yas/binary_iarchive.hpp>
#include <yas/types/std/pair.hpp>
#include <yas/types/std/string.hpp>
#include <yas/types/std/vector.hpp>
#include "EffectCompiler.h"
#include <regex>
#include "MagApp.h"
#include "DeviceResources.h"
#include "StrUtils.h"
#include "Logger.h"
#include "CommonSharedConstants.h"


//128bit multiply function
static void _wymum(uint64_t* A, uint64_t* B) {
	*A = _umul128(*A, *B, B);
}

//multiply and xor mix function, aka MUM
static uint64_t _wymix(uint64_t A, uint64_t B) { _wymum(&A, &B); return A ^ B; }

//read functions
static uint64_t _wyr8(const uint8_t* p) { uint64_t v; memcpy(&v, p, 8); return v; }
static uint64_t _wyr4(const uint8_t* p) { uint32_t v; memcpy(&v, p, 4); return v; }
static uint64_t _wyr3(const uint8_t* p, size_t k) { return (((uint64_t)p[0]) << 16) | (((uint64_t)p[k >> 1]) << 8) | p[k - 1]; }
//wyhash main function
static uint64_t wyhash(const void* key, size_t len) {
	uint64_t seed = 0;

	const uint8_t* p = (const uint8_t*)key;	uint64_t	a, b;
	if (len <= 16) {
		if (len >= 4) { a = (_wyr4(p) << 32) | _wyr4(p + ((len >> 3) << 2)); b = (_wyr4(p + len - 4) << 32) | _wyr4(p + len - 4 - ((len >> 3) << 2)); } else if (len > 0) { a = _wyr3(p, len); b = 0; } else a = b = 0;
	} else {
		size_t i = len;
		if (i > 48) {
			uint64_t see1 = seed, see2 = seed;
			do {
				seed = _wymix(_wyr8(p), _wyr8(p + 8) ^ seed);
				see1 = _wymix(_wyr8(p + 16), _wyr8(p + 24) ^ see1);
				see2 = _wymix(_wyr8(p + 32), _wyr8(p + 40) ^ see2);
				p += 48; i -= 48;
			} while (i > 48);
			seed ^= see1 ^ see2;
		}
		while (i > 16) { seed = _wymix(_wyr8(p), _wyr8(p + 8) ^ seed);  i -= 16; p += 16; }
		a = _wyr8(p + i - 16);  b = _wyr8(p + i - 8);
	}
	return _wymix(len, _wymix(a, b ^ seed));
}


template<typename Archive>
void serialize(Archive& ar, winrt::com_ptr<ID3DBlob>& o) {
	SIZE_T size = 0;
	ar& size;
	HRESULT hr = D3DCreateBlob(size, o.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("D3DCreateBlob 失败", hr);
		throw new std::exception();
	}

	BYTE* buf = (BYTE*)o->GetBufferPointer();
	for (SIZE_T i = 0; i < size; ++i) {
		ar& (*buf++);
	}
}

template<typename Archive>
void serialize(Archive& ar, const winrt::com_ptr<ID3DBlob>& o) {
	SIZE_T size = o->GetBufferSize();
	ar& size;

	BYTE* buf = (BYTE*)o->GetBufferPointer();
	for (SIZE_T i = 0; i < size; ++i) {
		ar& (*buf++);
	}
}

namespace Magpie::Runtime {

template<typename Archive>
void serialize(Archive& ar, const EffectParameterDesc& o) {
	size_t index = o.defaultValue.index();
	ar& index;

	if (index == 0) {
		ar& std::get<0>(o.defaultValue);
	} else {
		ar& std::get<1>(o.defaultValue);
	}

	ar& o.label;

	index = o.maxValue.index();
	ar& index;
	if (index == 1) {
		ar& std::get<1>(o.maxValue);
	} else if (index == 2) {
		ar& std::get<2>(o.maxValue);
	}

	index = o.minValue.index();
	ar& index;
	if (index == 1) {
		ar& std::get<1>(o.minValue);
	} else if (index == 2) {
		ar& std::get<2>(o.minValue);
	}

	ar& o.name& o.type;
}

template<typename Archive>
void serialize(Archive& ar, EffectParameterDesc& o) {
	size_t index = 0;
	ar& index;

	if (index == 0) {
		o.defaultValue.emplace<0>();
		ar& std::get<0>(o.defaultValue);
	} else {
		o.defaultValue.emplace<1>();
		ar& std::get<1>(o.defaultValue);
	}

	ar& o.label;

	ar& index;
	if (index == 0) {
		o.maxValue.emplace<0>();
	} else if (index == 1) {
		o.maxValue.emplace<1>();
		ar& std::get<1>(o.maxValue);
	} else {
		o.maxValue.emplace<2>();
		ar& std::get<2>(o.maxValue);
	}

	ar& index;
	if (index == 0) {
		o.minValue.emplace<0>();
	} else if (index == 1) {
		o.minValue.emplace<1>();
		ar& std::get<1>(o.minValue);
	} else {
		o.minValue.emplace<2>();
		ar& std::get<2>(o.minValue);
	}

	ar& o.name& o.type;
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
	ar& o.name& o.outSizeExpr& o.params& o.textures& o.samplers& o.passes& o.flags& o.isUseDynamic;
}


static constexpr const size_t MAX_CACHE_COUNT = 128;

// 缓存版本
// 当缓存文件结构有更改时更新它，使旧缓存失效
static constexpr const UINT CACHE_VERSION = 8;

// 缓存的压缩等级
static constexpr const int CACHE_COMPRESSION_LEVEL = 1;


static std::wstring GetCacheFileName(std::wstring_view effectName, std::wstring_view hash, UINT flags) {
	// 缓存文件的命名：{效果名}_{标志位（16进制）}{哈希}
	return fmt::format(L"{}{}_{:02x}_{}", CommonSharedConstants::CACHE_DIR_W, effectName, flags, hash);
}

void EffectCacheManager::_AddToMemCache(const std::wstring& cacheFileName, const EffectDesc& desc) {
	std::scoped_lock lk(_srwMutex);

	_memCache[cacheFileName] = { desc, ++_lastAccess };

	if (_memCache.size() > MAX_CACHE_COUNT) {
		// 清理一半较旧的内存缓存
		std::vector<UINT> access;
		access.reserve(_memCache.size());
		for (const auto& pair : _memCache) {
			access.push_back(pair.second.second);
		}

		auto midIt = access.begin() + access.size() / 2;
		std::nth_element(access.begin(), midIt, access.end());
		UINT mid = *midIt;

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

	std::wstring cacheFileName = GetCacheFileName(effectName, hash, desc.flags);

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

		if (!Utils::ZstdDecompress(compressedBuf, buf)) {
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


		if (!Utils::ZstdCompress(buf, compressedBuf, CACHE_COMPRESSION_LEVEL)) {
			Logger::Get().Error("压缩缓存失败");
			return;
		}
	}

	if (!Win32Utils::DirExists(CommonSharedConstants::CACHE_DIR_W)) {
		if (!CreateDirectory(CommonSharedConstants::CACHE_DIR_W, nullptr)) {
			Logger::Get().Win32Error("创建 cache 文件夹失败");
			return;
		}
	} else {
		// 删除所有该效果（flags 相同）的缓存
		std::wregex regex(fmt::format(L"^{}_{:02x}[0-9,a-f]{{{}}}$", effectName, desc.flags, 16),
			std::wregex::optimize | std::wregex::nosubs);

		WIN32_FIND_DATA findData{};
		HANDLE hFind = Win32Utils::SafeHandle(FindFirstFileEx(
			StrUtils::ConcatW(CommonSharedConstants::CACHE_DIR_W, L"*").c_str(),
			FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
		if (hFind) {
			do {
				if (StrUtils::StrLen(findData.cFileName) < 8) {
					continue;
				}

				// 正则匹配文件名
				if (!std::regex_match(findData.cFileName, regex)) {
					continue;
				}

				if (!DeleteFile(StrUtils::ConcatW(CommonSharedConstants::CACHE_DIR_W, findData.cFileName).c_str())) {
					Logger::Get().Win32Error(StrUtils::Concat("删除缓存文件 ",
						StrUtils::UTF16ToUTF8(findData.cFileName), " 失败"));
				}
			} while (FindNextFile(hFind, &findData));

			FindClose(hFind);
		} else {
			Logger::Get().Win32Error("查找缓存文件失败");
		}
	}

	std::wstring cacheFileName = GetCacheFileName(effectName, hash, desc.flags);
	if (!Win32Utils::WriteFile(cacheFileName.c_str(), compressedBuf.data(), compressedBuf.size())) {
		Logger::Get().Error("保存缓存失败");
	}

	_AddToMemCache(cacheFileName, desc);

	Logger::Get().Info(StrUtils::Concat("已保存缓存 ", StrUtils::UTF16ToUTF8(cacheFileName)));
}

static std::wstring DoHash(std::span<const BYTE> data) {
	uint64_t hashBytes = wyhash(data.data(), data.size());
	
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
	const std::unordered_map<std::wstring, float>* inlineParams
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

	return DoHash(std::span((const BYTE*)source.data(), source.size()));
}

std::wstring EffectCacheManager::GetHash(std::string& source, const std::unordered_map<std::wstring, float>* inlineParams) {
	size_t originSize = source.size();

	source.reserve(originSize + 256);

	source.append(fmt::format("CACHE_VERSION:{}\n", CACHE_VERSION));
	if (inlineParams) {
		for (const auto& pair : *inlineParams) {
			source.append(fmt::format("{}:{}\n", StrUtils::UTF16ToUTF8(pair.first), std::lroundf(pair.second * 10000)));
		}
	}

	std::wstring result = DoHash(std::span((const BYTE*)source.data(), source.size()));
	source.resize(originSize);
	return result;
}

}
