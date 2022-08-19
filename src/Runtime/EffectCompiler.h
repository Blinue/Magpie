#pragma once
#include "pch.h"
#include "EffectDesc.h"


namespace Magpie::Runtime {

struct EffectCompilerFlags {
	static constexpr const uint32_t NoCache = 0x1;
	static constexpr const uint32_t SaveSources = 0x2;
	static constexpr const uint32_t WarningsAreErrors = 0x4;
};

struct API_DECLSPEC EffectCompiler {
	// 调用者需填入 desc 中的 name 和 flags
	static uint32_t Compile(
		EffectDesc& desc,
		uint32_t flags,	// EffectCompilerFlags
		const std::unordered_map<std::wstring, float>* inlineParams = nullptr
	);

	// 当前 MagpieFX 版本
	static constexpr UINT VERSION = 2;
};

}
