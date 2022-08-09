#pragma once
#include "pch.h"
#include "EffectDesc.h"


namespace Magpie::Runtime {

class EffectCompiler {
public:
	EffectCompiler() = default;

	static UINT Compile(
		std::wstring_view effectName,
		UINT flags,
		const std::unordered_map<std::wstring, float>& inlineParams,
		EffectDesc& desc
	);

	// 当前 MagpieFX 版本
	static constexpr UINT VERSION = 2;
};

}
