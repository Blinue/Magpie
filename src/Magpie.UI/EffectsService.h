#pragma once
#include "pch.h"
#include <Magpie.Core.h>
#include "StrUtils.h"


namespace winrt::Magpie::UI {

struct EffectInfo {
	std::wstring name;
	std::vector<::Magpie::Core::EffectParameterDesc> params;
	bool canScale = false;
};

class EffectsService {
public:
	static EffectsService& Get() {
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
	std::unordered_map<std::wstring, uint32_t, StrUtils::StringHash<wchar_t>, std::equal_to<>> _effectsMap;
	std::atomic<bool> _initialized = false;
};

}
