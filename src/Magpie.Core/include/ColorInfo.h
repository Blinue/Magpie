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

	// 默认为 sRGB 原色和白点
	winrt::Point redPrimary = { 0.64f, 0.33f };
	winrt::Point greenPrimary = { 0.3f, 0.6f };
	winrt::Point bluePrimary = { 0.15f, 0.06f };
	winrt::Point whitePoint = { 0.3127f, 0.329f };

	bool operator==(const ColorInfo& other) const = default;
};

}
