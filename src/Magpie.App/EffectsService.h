#pragma once
#include <parallel_hashmap/phmap.h>

namespace Magpie::Core {
struct EffectParameterDesc;
}

namespace winrt::Magpie::App {

struct EffectInfoFlags {
	static constexpr uint32_t CanScale = 1;
};

struct EffectInfo {
	EffectInfo();
	~EffectInfo();

	std::wstring name;
	std::wstring sortName;
	std::vector<::Magpie::Core::EffectParameterDesc> params;
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

	fire_and_forget StartInitialize();

	void WaitForInitialize();

	const std::vector<EffectInfo>& Effects() const noexcept {
		return _effects;
	}

	const EffectInfo* GetEffect(std::wstring_view name) const noexcept {
		auto it = _effectsMap.find(name);
		return it != _effectsMap.end() ? &_effects[it->second] : nullptr;
	}

private:
	EffectsService() = default;

	std::vector<EffectInfo> _effects;
	phmap::flat_hash_map<std::wstring, uint32_t> _effectsMap;
	std::atomic<bool> _initialized = false;
};

}
