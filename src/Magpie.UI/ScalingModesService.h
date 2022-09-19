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

	// copyFrom < 0 表示新建空缩放配置
	void AddScalingMode(std::wstring_view name, int copyFrom);
private:
	ScalingModesService() = default;
};

}
