#pragma once

// forward needed screenshot api

#include <cstdint>
#include <vector>
#include <limits>
#include <winrt/base.h>

namespace Magpie {

struct EffectDesc;

class Renderer {
public:
	const std::vector<const EffectDesc*>& ActiveEffectDescs() const noexcept;
	winrt::fire_and_forget TakeScreenshot(
		uint32_t effectIdx,
		uint32_t passIdx = std::numeric_limits<uint32_t>::max(),
		uint32_t outputIdx = std::numeric_limits<uint32_t>::max()
	) noexcept;
};

class ScalingWindow {
public:
	static ScalingWindow& Get() noexcept;
	Renderer& Renderer() noexcept;
};

}
