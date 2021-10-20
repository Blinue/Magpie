#pragma once
#include "pch.h"
#include "StrUtils.h"
#include "Utils.h"
#include "EffectDesc.h"


class EffectCache {
public:
	static bool Load(const wchar_t* fileName, std::string_view hash, EffectDesc& desc);

	static void Save(const wchar_t* fileName, std::string_view hash, const EffectDesc& desc);

private:
	static std::wstring _GetCacheFileName(const wchar_t* fileName, std::string_view hash);

	// 缓存文件后缀名：Compiled MagpieFX
	static constexpr const wchar_t* _SUFFIX = L"cmfx";
};
