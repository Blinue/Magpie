#pragma once
#include <parallel_hashmap/phmap.h>

namespace Magpie {

struct EffectCompilerFlags {
	// 会影响编译出的字节码的标志放在低 16 位中，这样组织是为了便于缓存
	static constexpr uint32_t InlineParams = 1;
	static constexpr uint32_t FP16 = 1 << 1;

	// 只解析输出尺寸和参数，供用户界面使用
	static constexpr uint32_t NoCompile = 1 << 16;
	static constexpr uint32_t NoCache = 1 << 17;
	static constexpr uint32_t SaveSources = 1 << 18;
	static constexpr uint32_t WarningsAreErrors = 1 << 19;
	
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
