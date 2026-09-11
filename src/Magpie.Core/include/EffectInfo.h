#pragma once

namespace Magpie {

enum class EffectParameterType {
	Float,
	Int,
	UInt
	// TODO: 支持 bool
};

struct EffectParameterDesc {
	EffectParameterType type;
	std::string name;
	std::string label;

	float defaultValue;
	float minValue;
	float maxValue;
	float step;
};

enum class EffectFlags {
	None,
	SupportFP16 = 1,
	SupportAdvancedColor = 1 << 1,
	UseDynamic = 1 << 2
};
DEFINE_ENUM_FLAG_OPERATORS(EffectFlags)

struct EffectInfo {
	std::string name;
	std::string sortName;
	std::vector<EffectParameterDesc> params;
	// 0 表示可以自由缩放
	uint32_t scaleFactor = 0;
	// 0 表示不限制
	uint32_t minFrameRate = 0;
	EffectFlags flags = EffectFlags::None;
};

}
