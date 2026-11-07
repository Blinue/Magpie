#pragma once
#include "SmallVector.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie {

struct EffectInfo;
struct ShaderEffectDrawInfo;

enum class ShaderEffectParserFlags {
	None = 0,
	// EnableMinFloat16 和 EnableNative16Bit 不能同时出现
	EnableMinFloat16 = 1,
	EnableNative16Bit = 1 << 1,
	EnableAdvancedColor = 1 << 2
};
DEFINE_ENUM_FLAG_OPERATORS(ShaderEffectParserFlags)

struct ShaderEffectParserOptions {
	// 为空表示不内联
	const phmap::flat_hash_map<std::string, float>* inlineParams = nullptr;
	D3D_SHADER_MODEL shaderModel = D3D_SHADER_MODEL_5_1;
	ShaderEffectParserFlags flags = ShaderEffectParserFlags::None;
};

struct ShaderEffectSource {
	std::string source;
	SmallVector<std::pair<std::string, std::string>, 0> macros;
};

struct ShaderEffectParser {
	// 成功时返回空字符串，否则返回错误消息
	static std::string ParseForInfo(
		std::string&& name,
		std::string&& source,
		EffectInfo& effectInfo
	) noexcept;

	static std::string ParseForDesc(
		const EffectInfo& effectInfo,
		std::string&& source,
		const ShaderEffectParserOptions& options,
		ShaderEffectDrawInfo& drawInfo,
		SmallVectorImpl<ShaderEffectSource>& effectSources
	) noexcept;
};

}
