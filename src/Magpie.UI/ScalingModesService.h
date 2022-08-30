#pragma once
#include "pch.h"
#include "Magpie.Core.h"


class ScalingModesService {
public:
	static ScalingModesService& Get() {
		static ScalingModesService instance;
		return instance;
	}

	ScalingModesService(const ScalingModesService&) = delete;
	ScalingModesService(ScalingModesService&&) = delete;
private:
	ScalingModesService() = default;
};
