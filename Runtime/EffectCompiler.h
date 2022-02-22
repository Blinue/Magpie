#pragma once
#include "pch.h"
#include "EffectDesc.h"


class EffectCompiler {
public:
	EffectCompiler() = default;

	static UINT Compile(const wchar_t* fileName, EffectDesc& desc, UINT flags = 0);

	// 当前 MagpieFX 版本
	static constexpr UINT VERSION = 2;
};
