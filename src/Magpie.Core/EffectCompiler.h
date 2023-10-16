#pragma once
#include <parallel_hashmap/phmap.h>

namespace Magpie::Core {

struct EffectDesc;

struct EffectCompilerFlags {
	static constexpr const uint32_t NoCache = 0x1;
	static constexpr const uint32_t SaveSources = 0x2;
	static constexpr const uint32_t WarningsAreErrors = 0x4;
	// 只解析输出尺寸和参数，供用户界面使用
	static constexpr const uint32_t NoCompile = 0x8;
};

struct EffectCompiler {
	// 调用者需填入 desc 中的 name 和 flags
	static uint32_t Compile(
		EffectDesc& desc,
		uint32_t flags,	// EffectCompilerFlags
		const phmap::flat_hash_map<std::wstring, float>* inlineParams = nullptr
	);

	// 当前 MagpieFX 版本
	static constexpr UINT VERSION = 3;
};

}
