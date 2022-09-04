#pragma once
#include "pch.h"
#include "Magpie.Core.h"
#include <ScalingMode.h>


namespace winrt::Magpie::UI {

class ScalingModesService {
public:
	static ScalingModesService& Get() {
		static ScalingModesService instance;
		return instance;
	}

	ScalingModesService(const ScalingModesService&) = delete;
	ScalingModesService(ScalingModesService&&) = delete;

	ScalingMode& GetScalingMode(uint32_t idx);

	uint32_t GetScalingModeCount();
private:
	ScalingModesService() = default;
};

}
