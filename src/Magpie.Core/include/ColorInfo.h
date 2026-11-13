#pragma once

namespace Magpie {

struct ColorInfo {
	winrt::AdvancedColorKind kind = winrt::AdvancedColorKind::StandardDynamicRange;

	// HDR 模式下最低亮度，1.0 表示 80nit
	float minLuminance = 0.0f;
	// HDR 模式下最高亮度，1.0 表示 80nit
	float maxLuminance = 1.0f;
	// HDR 模式下 SDR 内容亮度，1.0 表示 80nit
	float sdrWhiteLevel = 1.0f;

	bool operator==(const ColorInfo& other) const = default;
};

}
