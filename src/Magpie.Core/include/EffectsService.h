#pragma once
#include "EffectInfo.h"
#include "Singleton.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie {

struct ShaderEffectDrawInfo;

class EffectsService : public Singleton<EffectsService> {
	friend Singleton<EffectsService>;

public:
	winrt::fire_and_forget Initialize();

	void Uninitialize() noexcept;

	const std::vector<EffectInfo>& GetEffects() noexcept;

	const EffectInfo* GetEffect(std::string_view name) noexcept;

	std::string SubmitCompileShaderEffectTask(
		std::string_view effectName,
		const phmap::flat_hash_map<std::string, float>* inlineParams,
		D3D_SHADER_MODEL shaderModel,
		bool isMinFloat16Supported,
		bool isNative16BitSupported,
		bool isAdvancedColorSupported,
		bool saveSources,
		bool warningsAreErrors,
		bool disableCache
	) noexcept;

	bool GetTaskResult(const std::string& taskKey, const ShaderEffectDrawInfo** drawInfo) noexcept;

	void ReleaseTask(const std::string& taskKey) noexcept;

private:
	EffectsService();
	~EffectsService();

	void _WaitForInitialize() noexcept;

	winrt::fire_and_forget _CompileShaderEffectAsync(
		std::string effectName,
		std::string source,
		const phmap::flat_hash_map<std::string, float>* inlineParams,
		D3D_SHADER_MODEL shaderModel,
		std::string cacheKey,
		uint32_t parserFlags,
		bool saveSources,
		bool warningsAreErrors,
		bool disableCache
	) noexcept;

	std::vector<EffectInfo> _effects;
	phmap::flat_hash_map<std::string_view, uint32_t> _effectsMap;

	// 定义在实现文件
	struct _ShaderEffectMemCacheItem;
	// 需确保 _ShaderEffectMemCacheItem::drawInfo 地址稳定
	phmap::node_hash_map<std::string, _ShaderEffectMemCacheItem> _shaderEffectCache;
	wil::srwlock _shaderEffectCacheLock;
	uint32_t _nextLastAccess = 0;

	// 用于后台线程检查 Uninitialize 是否已被调用
	struct _StopSource {
		wil::srwlock lock;
		bool isUninitialized = false;
	};
	std::shared_ptr<_StopSource> _stopSource = std::make_shared<_StopSource>();
	
	std::atomic<bool> _initialized = false;
	bool _initializedCache = false;
};

}
