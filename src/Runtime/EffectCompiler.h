#pragma once
#include "pch.h"
#include "EffectDesc.h"


namespace Magpie::Runtime {

class EffectCompiler {
public:
	EffectCompiler() = default;

	static UINT Compile(
		std::string_view effectName,
		UINT flags,
		const std::map<std::string, std::variant<float, int>>& inlineParams,
		EffectDesc& desc
	);

	// 当前 MagpieFX 版本
	static constexpr UINT VERSION = 2;
};

}
