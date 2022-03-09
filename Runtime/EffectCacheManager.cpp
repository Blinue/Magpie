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
#include "App.h"
#include "DeviceResources.h"
#include "StrUtils.h"
#include "Logger.h"


static constexpr const size_t MAX_CACHE_COUNT = 128;

// 缓存版本
// 当缓存文件结构有更改时将更新它，使得所有旧缓存失效
static constexpr const UINT CACHE_VERSION = 5;

// 缓存的压缩等级
static constexpr const int CACHE_COMPRESSION_LEVEL = 1;

static const wchar_t* CACHE_DIR = L".\\cache";


std::wstring GetCacheFileName(std::string_view effectName, std::string_view hash, UINT flags) {
	// 缓存文件的命名：{效果名}_{标志位（16进制）}{哈希}
	return fmt::format(L"{}\\{}_{:02x}{}", CACHE_DIR, StrUtils::UTF8ToUTF16(effectName), flags, StrUtils::UTF8ToUTF16(hash));
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
	ar& o.format& o.name & o.source & o.sizeExpr;
}

template<typename Archive>
void serialize(Archive& ar, EffectSamplerDesc& o) {
	ar& o.filterType& o.addressType& o.name;
}

template<typename Archive>
void serialize(Archive& ar, EffectPassDesc& o) {
	ar& o.cso& o.inputs& o.outputs& o.numThreads[0] & o.numThreads[1] & o.numThreads[2] & o.blockSize& o.isPSStyle;
}

template<typename Archive>
void serialize(Archive& ar, EffectDesc& o) {
	ar& o.name& o.outSizeExpr& o.params& o.textures& o.samplers& o.passes& o.flags;
}

void EffectCacheManager::_AddToMemCache(const std::wstring& cacheFileName, const EffectDesc& desc) {
	std::scoped_lock lk(_cs);

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
	std::scoped_lock lk(_cs);
	
	auto it = _memCache.find(cacheFileName);
	if (it != _memCache.end()) {
		desc = it->second.first;
		it->second.second = ++_lastAccess;
		Logger::Get().Info(StrUtils::Concat("已读取缓存 ", StrUtils::UTF16ToUTF8(cacheFileName)));
		return true;
	}
	return false;
}

bool EffectCacheManager::Load(std::string_view effectName, std::string_view hash, EffectDesc& desc) {
	assert(!effectName.empty() && !hash.empty());

	std::wstring cacheFileName = GetCacheFileName(effectName, hash, desc.flags);

	if (_LoadFromMemCache(cacheFileName, desc)) {
		return true;
	}

	if (!Utils::FileExists(cacheFileName.c_str())) {
		return false;
	}
	
	std::vector<BYTE> buf;
	{
		std::vector<BYTE> compressedBuf;
		if (!Utils::ReadFile(cacheFileName.c_str(), compressedBuf) || compressedBuf.empty()) {
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

void EffectCacheManager::Save(std::string_view effectName, std::string_view hash, const EffectDesc& desc) {
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
	
	if (!Utils::DirExists(CACHE_DIR)) {
		if (!CreateDirectory(CACHE_DIR, nullptr)) {
			Logger::Get().Win32Error("创建 cache 文件夹失败");
			return;
		}
	} else {
		// 删除所有该效果（flags 相同）的缓存
		std::wregex regex(fmt::format(L"^{}_{:02x}[0-9,a-f]{{{}}}$", StrUtils::UTF8ToUTF16(effectName), desc.flags,
				Utils::Hasher::Get().GetHashLength() * 2), std::wregex::optimize | std::wregex::nosubs);

		WIN32_FIND_DATA findData;
		HANDLE hFind = Utils::SafeHandle(FindFirstFile(StrUtils::ConcatW(CACHE_DIR, L"\\*").c_str(), &findData));
		if (hFind) {
			while (FindNextFile(hFind, &findData)) {
				if (StrUtils::StrLen(findData.cFileName) < 8) {
					continue;
				}

				// 正则匹配文件名
				if (!std::regex_match(findData.cFileName, regex)) {
					continue;
				}

				if (!DeleteFile(StrUtils::ConcatW(CACHE_DIR, L"\\", findData.cFileName).c_str())) {
					Logger::Get().Win32Error(StrUtils::Concat("删除缓存文件 ",
						StrUtils::UTF16ToUTF8(findData.cFileName), " 失败"));
				}
			}
			FindClose(hFind);
		} else {
			Logger::Get().Win32Error("查找缓存文件失败");
		}
	}
	
	std::wstring cacheFileName = GetCacheFileName(effectName, hash, desc.flags);
	if (!Utils::WriteFile(cacheFileName.c_str(), compressedBuf.data(), compressedBuf.size())) {
		Logger::Get().Error("保存缓存失败");
	}

	_AddToMemCache(cacheFileName, desc);

	Logger::Get().Info(StrUtils::Concat("已保存缓存 ", StrUtils::UTF16ToUTF8(cacheFileName)));
}

std::string EffectCacheManager::GetHash(
	std::string_view source,
	const std::map<std::string, std::variant<float, int>>* inlineParams
) {
	std::string str;
	str.reserve(source.size() + 128);
	str = source;

	str.append(fmt::format("CACHE_VERSION:{}\n", CACHE_VERSION));
	if (inlineParams) {
		for (const auto& pair : *inlineParams) {
			if (pair.second.index() == 0) {
				str.append(fmt::format("{}:{}\n", pair.first, std::get<0>(pair.second)));
			} else {
				str.append(fmt::format("{}:{}\n", pair.first, std::get<1>(pair.second)));
			}
		}
	}

	std::vector<BYTE> hashBytes;
	if (!Utils::Hasher::Get().Hash(std::span((const BYTE*)source.data(), source.size()), hashBytes)) {
		Logger::Get().Error("计算 hash 失败");
		return "";
	}

	return Utils::Bin2Hex(hashBytes);
}

std::string EffectCacheManager::GetHash(std::string& source, const std::map<std::string, std::variant<float, int>>* inlineParams) {
	size_t originSize = source.size();

	source.reserve(originSize + 128);

	source.append(fmt::format("CACHE_VERSION:{}\n", CACHE_VERSION));
	if (inlineParams) {
		for (const auto& pair : *inlineParams) {
			if (pair.second.index() == 0) {
				source.append(fmt::format("{}:{}\n", pair.first, std::get<0>(pair.second)));
			} else {
				source.append(fmt::format("{}:{}\n", pair.first, std::get<1>(pair.second)));
			}
		}
	}

	std::vector<BYTE> hashBytes;
	bool success = Utils::Hasher::Get().Hash(std::span((const BYTE*)source.data(), source.size()), hashBytes);
	if (!success) {
		Logger::Get().Error("计算 hash 失败");
	}

	source.resize(originSize);

	return success ? Utils::Bin2Hex(hashBytes) : "";
}
