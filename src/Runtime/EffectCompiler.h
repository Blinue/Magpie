#pragma once
#include "pch.h"
#include "EffectDesc.h"


namespace Magpie::Runtime {

struct API_DECLSPEC EffectCompiler {
	static uint32_t Compile(
		std::wstring_view effectName,
		EffectDesc& desc,
		uint32_t flags,
		const std::unordered_map<std::wstring, float>* inlineParams = nullptr
	);

	// 当前 MagpieFX 版本
	static constexpr UINT VERSION = 2;
};

}
