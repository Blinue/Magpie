#pragma once
#include "Win32Utils.h"
#include "EffectDesc.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie::Core {

class EffectCacheManager {
public:
	static EffectCacheManager& Get() noexcept {
		static EffectCacheManager instance;
		return instance;
	}

	EffectCacheManager(const EffectCacheManager&) = delete;
	EffectCacheManager(EffectCacheManager&&) = delete;

	bool Load(std::wstring_view effectName, std::wstring_view hash, EffectDesc& desc);

	void Save(std::wstring_view effectName, std::wstring_view hash, const EffectDesc& desc);

	// inlineParams 为内联变量，可以为空
	// 接受 std::string& 的重载速度更快，且保证不修改 source
	static std::wstring GetHash(
		std::string_view source,
		const phmap::flat_hash_map<std::wstring, float>* inlineParams = nullptr
	);
	static std::wstring GetHash(
		std::string& source,
		const phmap::flat_hash_map<std::wstring, float>* inlineParams = nullptr
	);

private:
	EffectCacheManager() = default;

	void _AddToMemCache(const std::wstring& cacheFileName, const EffectDesc& desc);
	bool _LoadFromMemCache(const std::wstring& cacheFileName, EffectDesc& desc);

	// 用于同步对 _memCache 的访问
	wil::srwlock _lock;
	// cacheFileName -> (EffectDesc, lastAccess)
	phmap::flat_hash_map<std::wstring, std::pair<EffectDesc, UINT>> _memCache;
	UINT _lastAccess = 0;
};

}
