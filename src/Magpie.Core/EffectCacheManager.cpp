#include "pch.h"
#include "EffectCacheManager.h"
#include "StrHelper.h"
#include "Logger.h"
#include "CommonSharedConstants.h"
#include <d3dcompiler.h>
#include <rapidhash.h>
#include "YasHelper.h"

namespace yas::detail {

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

		ar.write(blob->GetBufferPointer(), size);

		return ar;
	}

	template<typename Archive>
	static Archive& load(Archive& ar, winrt::com_ptr<ID3DBlob>& blob) {
		uint32_t size = 0;
		ar& size;
		HRESULT hr = D3DCreateBlob(size, blob.put());
		if (FAILED(hr)) {
			Magpie::Logger::Get().ComError("D3DCreateBlob 失败", hr);
			throw new std::exception();
		}

		ar.read(blob->GetBufferPointer(), size);

		return ar;
	}
};

}

namespace Magpie {

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
	ar& o.cso& o.inputs& o.outputs& o.numThreads[0] & o.numThreads[1] & o.numThreads[2] & o.blockSize& o.desc& o.flags;
}

template<typename Archive>
void serialize(Archive& ar, EffectDesc& o) {
	ar& o.name& o.params& o.textures& o.samplers& o.passes& o.flags;
}

static constexpr uint32_t MAX_CACHE_COUNT = 127;

// 缓存版本
// 当缓存文件结构有更改时更新它，使旧缓存失效
static constexpr uint32_t EFFECT_CACHE_VERSION = 15;


static std::wstring GetLinearEffectName(std::wstring_view effectName) {
	std::wstring result(effectName);
	for (wchar_t& c : result) {
		if (c == L'\\') {
			c = L'#';
		}
	}
	return result;
}

static std::wstring GetCacheFileName(std::wstring_view linearEffectName, uint32_t flags, uint64_t hash) {
	assert(flags <= 0xFFFF);
	// 缓存文件的命名: {效果名}_{标志位(4)}_{哈希(16)）}
	return fmt::format(L"{}{}_{:04x}_{:016x}", CommonSharedConstants::CACHE_DIR, linearEffectName, flags, hash);
}

void EffectCacheManager::_AddToMemCache(const std::wstring& cacheFileName, std::string& key, const EffectDesc& desc) {
	auto lock = _lock.lock_exclusive();

	_memCache[cacheFileName] = _MemCacheItem{
		.key = std::move(key),
		.effectDesc = desc,
		.lastAccess = ++_lastAccess
	};

	if (_memCache.size() > MAX_CACHE_COUNT) {
		assert(_memCache.size() == MAX_CACHE_COUNT + 1);

		// 清理一半较旧的内存缓存
		std::array<uint32_t, MAX_CACHE_COUNT + 1> access{};
		std::transform(_memCache.begin(), _memCache.end(), access.begin(),
			[](const auto& pair) {return pair.second.lastAccess; });

		auto midIt = access.begin() + access.size() / 2;
		std::nth_element(access.begin(), midIt, access.end());
		const uint32_t mid = *midIt;

		for (auto it = _memCache.begin(); it != _memCache.end();) {
			if (it->second.lastAccess < mid) {
				it = _memCache.erase(it);
			} else {
				++it;
			}
		}

		Logger::Get().Info("已清理内存缓存");
	}
}

bool EffectCacheManager::_LoadFromMemCache(const std::wstring& cacheFileName, std::string_view key, EffectDesc& desc) {
	auto lock = _lock.lock_exclusive();

	auto it = _memCache.find(cacheFileName);
	if (it != _memCache.end()) {
		_MemCacheItem& cacheItem = it->second;

		// 防止哈希碰撞
		if (cacheItem.key != key) {
			return false;
		}

		desc = cacheItem.effectDesc;
		cacheItem.lastAccess = ++_lastAccess;
		Logger::Get().Info(StrHelper::Concat("已读取缓存 ", StrHelper::UTF16ToUTF8(cacheFileName)));
		return true;
	}
	return false;
}

bool EffectCacheManager::Load(
	std::wstring_view effectName,
	uint32_t flags,
	uint64_t hash,
	std::string_view key,
	EffectDesc& desc
) {
	assert(!effectName.empty() && !key.empty());

	std::wstring cacheFileName = GetCacheFileName(GetLinearEffectName(effectName), flags, hash);

	if (_LoadFromMemCache(cacheFileName, key, desc)) {
		return true;
	}

	if (!Win32Helper::FileExists(cacheFileName.c_str())) {
		return false;
	}

	std::vector<BYTE> buf;
	if (!Win32Helper::ReadFile(cacheFileName.c_str(), buf) || buf.empty()) {
		return false;
	}

	std::string cachedKey;
	try {
		yas::mem_istream mi(buf.data(), buf.size());
		yas::binary_iarchive<yas::mem_istream, yas::binary> ia(mi);

		uint32_t cacheVersion;
		ia.read(cacheVersion);
		if (cacheVersion != EFFECT_CACHE_VERSION) {
			Logger::Get().Info("缓存版本不匹配");
			return false;
		}
		
		ia& cachedKey;
		if (cachedKey != key) {
			Logger::Get().Info("缓存键不匹配");
			return false;
		}

		ia& desc;
	} catch (...) {
		Logger::Get().Error("反序列化失败");
		desc = {};
		return false;
	}

	_AddToMemCache(cacheFileName, cachedKey, desc);

	Logger::Get().Info(StrHelper::Concat("已读取缓存 ", StrHelper::UTF16ToUTF8(cacheFileName)));
	return true;
}

void EffectCacheManager::Save(
	std::wstring_view effectName,
	uint32_t flags,
	uint64_t hash,
	std::string key,
	const EffectDesc& desc
) {
	const std::wstring linearEffectName = GetLinearEffectName(effectName);

	std::vector<BYTE> buf;
	buf.reserve(4096);

	try {
		yas::vector_ostream os(buf);
		yas::binary_oarchive<yas::vector_ostream<BYTE>, yas::binary> oa(os);

		oa.write(EFFECT_CACHE_VERSION);
		oa& key& desc;
	} catch (...) {
		Logger::Get().Error("序列化 EffectDesc 失败");
		return;
	}

	if (!CreateDirectory(CommonSharedConstants::CACHE_DIR, nullptr)) {
		if (GetLastError() != ERROR_ALREADY_EXISTS) {
			Logger::Get().Win32Error("创建 cache 文件夹失败");
			return;
		}

		// 清理缓存
		WIN32_FIND_DATA findData{};
		wil::unique_hfind hFind(FindFirstFileEx(
			StrHelper::Concat(CommonSharedConstants::CACHE_DIR, L"*").c_str(),
			FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
		if (hFind) {
			do {
				std::wstring_view fileName(findData.cFileName);

				if (!fileName.starts_with(linearEffectName)) {
					continue;
				}

				const size_t effectNameLen = linearEffectName.size();
				if (fileName.size() == effectNameLen + 22) {
					// 保留标志不同的缓存
					if (!fileName.substr(effectNameLen).starts_with(fmt::format(L"_{:04x}_", flags))) {
						continue;
					}

					int i = 6;
					for (; i < 22; ++i) {
						const wchar_t c = fileName[effectNameLen + i];
						if (!(c >= L'0' && c <= L'9' || c >= L'a' && c <= L'f')) {
							break;
						}
					}
					if (i != 22) {
						continue;
					}
				} else if (fileName.size() == effectNameLen + 18) {
					// 删除旧版缓存
					if (fileName[effectNameLen] != L'_') {
						continue;
					}

					int i = 1;
					for (; i < 18; ++i) {
						const wchar_t c = fileName[effectNameLen + i];
						if (!(c >= L'0' && c <= L'9' || c >= L'a' && c <= L'f')) {
							break;
						}
					}
					if (i != 18) {
						continue;
					}
				} else {
					continue;
				}

				if (!DeleteFile(StrHelper::Concat(CommonSharedConstants::CACHE_DIR, findData.cFileName).c_str())) {
					Logger::Get().Win32Error(StrHelper::Concat("删除缓存文件 ",
						StrHelper::UTF16ToUTF8(findData.cFileName), " 失败"));
				}
			} while (FindNextFile(hFind.get(), &findData));
		} else {
			Logger::Get().Win32Error("查找缓存文件失败");
		}
	}

	std::wstring cacheFileName = GetCacheFileName(linearEffectName, flags, hash);
	if (!Win32Helper::WriteFile(cacheFileName.c_str(), buf.data(), buf.size())) {
		Logger::Get().Error("保存缓存失败");
	}

	_AddToMemCache(cacheFileName, key, desc);

	Logger::Get().Info(StrHelper::Concat("已保存缓存 ", StrHelper::UTF16ToUTF8(cacheFileName)));
}

uint64_t EffectCacheManager::GetHash(std::string_view key) {
	return rapidhash(key.data(), key.size());
}

}
