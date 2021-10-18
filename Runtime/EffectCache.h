#pragma once
#include "pch.h"
#include "StrUtils.h"
#include "Utils.h"
#include "EffectDesc.h"


class EffectCache {
public:
	static bool Load(const wchar_t* fileName, std::string_view hash, EffectDesc& desc);

	static void Save(const wchar_t* fileName, std::string_view hash, const EffectDesc& desc);
};
