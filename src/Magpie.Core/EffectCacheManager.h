#pragma once
#include "EffectDesc.h"
#include "LruMemoryCache.h"
#include "Singleton.h"

namespace Magpie {

class EffectCacheManager : public Singleton<EffectCacheManager> {
	friend Singleton<EffectCacheManager>;

public:
	bool Load(std::wstring_view effectName, uint32_t flags, uint64_t hash, std::string_view key, EffectDesc& desc);

	void Save(std::wstring_view effectName, uint32_t flags, uint64_t hash, std::string key, const EffectDesc& desc);

	static uint64_t GetHash(std::string_view key);

private:
	EffectCacheManager() = default;

	// 用于同步对 _memCache 的访问
	wil::srwlock _lock;

	LruMemoryCache<std::wstring, EffectDesc, 3> _memCache;
};

}
