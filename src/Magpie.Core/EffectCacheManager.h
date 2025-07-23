#pragma once
#include "EffectDesc.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie {

class EffectCacheManager {
public:
	static EffectCacheManager& Get() noexcept {
		static EffectCacheManager instance;
		return instance;
	}

	EffectCacheManager(const EffectCacheManager&) = delete;
	EffectCacheManager(EffectCacheManager&&) = delete;

	bool Load(std::wstring_view effectName, uint32_t flags, uint64_t hash, std::string_view key, EffectDesc& desc);

	void Save(std::wstring_view effectName, uint32_t flags, uint64_t hash, std::string key, const EffectDesc& desc);

	static uint64_t GetHash(std::string_view key);

private:
	EffectCacheManager() = default;

	void _AddToMemCache(const std::wstring& cacheFileName, std::string& key, const EffectDesc& desc);
	bool _LoadFromMemCache(const std::wstring& cacheFileName, std::string_view key, EffectDesc& desc);

	// 用于同步对 _memCache 的访问
	wil::srwlock _lock;

	struct _MemCacheItem {
		std::string key;
		EffectDesc effectDesc;
		uint32_t lastAccess = 0;
	};
	phmap::flat_hash_map<std::wstring, _MemCacheItem> _memCache;

	UINT _lastAccess = 0;
};

}
