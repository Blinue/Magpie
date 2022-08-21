#pragma once
#include "pch.h"
#include <Runtime.h>


namespace winrt::Magpie::App {

struct EffectInfo {
	std::wstring name;
	std::vector<::Magpie::Runtime::EffectParameterDesc> params;
	bool hasScale = false;
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

private:
	EffectsService() = default;

	std::vector<EffectInfo> _effects;
	std::atomic<bool> _initialized = false;
};

}
