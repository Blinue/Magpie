#pragma once
#include "ExportHelper.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie::Core {

struct EffectCompilerFlags {
	static constexpr const uint32_t NoCache = 1;
	static constexpr const uint32_t SaveSources = 1 << 1;
	static constexpr const uint32_t WarningsAreErrors = 1 << 2;
	// 只解析输出尺寸和参数，供用户界面使用
	static constexpr const uint32_t NoCompile = 1 << 3;
};

struct API_DECLSPEC EffectCompiler {
	// 调用者需填入 desc 中的 name 和 flags
	static uint32_t Compile(
		struct EffectDesc& desc,
		uint32_t flags,	// EffectCompilerFlags
		const phmap::flat_hash_map<std::wstring, float>* inlineParams = nullptr
	) noexcept;

	// 将这些效果内置防止没有默认降采样效果
	static constexpr const wchar_t* BUILTIN_EFFECTS[] = { L"Bicubic" };
};

}
