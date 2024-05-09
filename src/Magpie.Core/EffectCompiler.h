#pragma once
#include <parallel_hashmap/phmap.h>

namespace Magpie::Core {

struct EffectCompilerFlags {
	static constexpr uint32_t NoCache = 1;
	static constexpr uint32_t SaveSources = 1 << 1;
	static constexpr uint32_t WarningsAreErrors = 1 << 2;
	// 只解析输出尺寸和参数，供用户界面使用
	static constexpr uint32_t NoCompile = 1 << 3;
};

struct EffectCompiler {
	// 调用者需填入 desc 中的 name 和 flags
	static uint32_t Compile(
		struct EffectDesc& desc,
		uint32_t flags,	// EffectCompilerFlags
		const phmap::flat_hash_map<std::wstring, float>* inlineParams = nullptr
	) noexcept;
};

}
