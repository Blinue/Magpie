#pragma once
#include "pch.h"
#include "Utils.h"
#include "EffectDesc.h"


class EffectCacheManager {
public:
	static EffectCacheManager& Get() {
		static EffectCacheManager instance;
		return instance;
	}

	bool Load(std::string_view effectName, std::string_view hash, EffectDesc& desc);

	void Save(std::string_view effectName, std::string_view hash, const EffectDesc& desc);

	// inlineParams 为内联变量，可以为空
	// 接受 std::string& 的重载速度更快，且保证不修改 source
	static std::string GetHash(
		std::string_view source,
		const std::map<std::string, std::variant<float, int>>* inlineParams = nullptr
	);
	static std::string GetHash(
		std::string& source,
		const std::map<std::string, std::variant<float, int>>* inlineParams = nullptr
	);

private:
	void _AddToMemCache(const std::wstring& cacheFileName, const EffectDesc& desc);

	std::unordered_map<std::wstring, EffectDesc> _memCache;

	static constexpr const size_t _MAX_CACHE_COUNT = 100;

	static std::wstring _GetCacheFileName(std::string_view effectName, std::string_view hash);

	// 缓存文件后缀名：Compiled MagpieFX
	static constexpr const wchar_t* _SUFFIX = L"cmfx";

	// 缓存版本
	// 当缓存文件结构有更改时将更新它，使得所有旧缓存失效
	static constexpr const UINT _VERSION = 3;
};
