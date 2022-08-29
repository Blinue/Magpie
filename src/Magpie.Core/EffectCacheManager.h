#pragma once
#include "pch.h"
#include "Win32Utils.h"
#include "EffectDesc.h"


namespace Magpie::Runtime {

class EffectCacheManager {
public:
	static EffectCacheManager& Get() {
		static EffectCacheManager instance;
		return instance;
	}

	bool Load(std::wstring_view effectName, std::wstring_view hash, EffectDesc& desc);

	void Save(std::wstring_view effectName, std::wstring_view hash, const EffectDesc& desc);

	// inlineParams 为内联变量，可以为空
	// 接受 std::string& 的重载速度更快，且保证不修改 source
	static std::wstring GetHash(
		std::string_view source,
		const std::unordered_map<std::wstring, float>* inlineParams = nullptr
	);
	static std::wstring GetHash(
		std::string& source,
		const std::unordered_map<std::wstring, float>* inlineParams = nullptr
	);

private:
	void _AddToMemCache(const std::wstring& cacheFileName, const EffectDesc& desc);
	bool _LoadFromMemCache(const std::wstring& cacheFileName, EffectDesc& desc);

	// 用于同步对 _memCache 的访问
	Win32Utils::SRWMutex _srwMutex;
	// cacheFileName -> (EffectDesc, lastAccess)
	std::unordered_map<std::wstring, std::pair<EffectDesc, UINT>> _memCache;
	UINT _lastAccess = 0;
};

}
