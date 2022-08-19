#pragma once
#include "pch.h"


namespace winrt::Magpie::App {

class EffectsService {
public:
	static EffectsService& Get() {
		static EffectsService instance;
		return instance;
	}

	void Initialize();

private:
	EffectsService() = default;

	EffectsService(const EffectsService&) = delete;
	EffectsService(EffectsService&&) = delete;
};

}
