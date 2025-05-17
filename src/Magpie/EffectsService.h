#pragma once
#include <parallel_hashmap/phmap.h>

namespace Magpie {

struct EffectParameterDesc;

struct EffectInfoFlags {
	static constexpr uint32_t CanScale = 1;
};

struct EffectInfo {
	EffectInfo();
	~EffectInfo();

	std::wstring name;
	std::wstring sortName;
	std::vector<EffectParameterDesc> params;
	uint32_t flags = 0;	// EffectInfoFlags

	bool CanScale() const noexcept {
		return flags & EffectInfoFlags::CanScale;
	}
};

class EffectsService {
public:
	static EffectsService& Get() noexcept {
		static EffectsService instance;
		return instance;
	}

	EffectsService(const EffectsService&) = delete;
	EffectsService(EffectsService&&) = delete;

	winrt::fire_and_forget Initialize();

	void Uninitialize();

	const std::vector<EffectInfo>& Effects() noexcept;

	const EffectInfo* GetEffect(std::wstring_view name) noexcept;

private:
	EffectsService() = default;

	void _WaitForInitialize() noexcept;

	std::vector<EffectInfo> _effects;
	phmap::flat_hash_map<std::wstring, uint32_t> _effectsMap;
	std::atomic<bool> _initialized = false;
	bool _initializedCache = false;
};

}
