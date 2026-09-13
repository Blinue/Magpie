#pragma once
#include <parallel_hashmap/phmap.h>

namespace Magpie {

enum class EffectCompilerFlags : uint32_t {
	// 会影响编译出的字节码的标志放在低 16 位中，这样组织是为了便于缓存
	None,
	InlineParams = 1,
	NoFP16 = 1 << 1,
	// 只解析输出尺寸和参数，供用户界面使用
	NoCompile = 1 << 16,
	NoCache = 1 << 17,
	SaveSources = 1 << 18,
	WarningsAreErrors = 1 << 19
};
DEFINE_ENUM_FLAG_OPERATORS(EffectCompilerFlags)

struct EffectCompiler {
	// 调用者需填入 desc 中的 name 和 flags
	static uint32_t Compile(
		struct EffectDesc& desc,
		EffectCompilerFlags flags,
		const phmap::flat_hash_map<std::string, float>* inlineParams = nullptr
	) noexcept;
};

}
