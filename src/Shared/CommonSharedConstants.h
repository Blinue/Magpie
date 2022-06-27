#pragma once
#include "CommonPCH.h"


struct CommonSharedConstants {
	static constexpr const COLORREF LIGHT_TINT_COLOR = RGB(243, 243, 243);
	static constexpr const COLORREF DARK_TINT_COLOR = RGB(32, 32, 32);

	static constexpr const char* LOG_PATH = "logs\\magpie.log";
	static constexpr const char* CONFIG_PATH = "config\\config.json";
	static constexpr const wchar_t* CONFIG_PATH_W = L"config\\config.json";
	static constexpr const wchar_t* SOURCES_DIR_W = L"sources\\";
	static constexpr const wchar_t* EFFECTS_DIR_W = L"effects\\";
	static constexpr const wchar_t* ASSETS_DIR_W = L"assets\\";
	static constexpr const wchar_t* CACHE_DIR_W = L"cache\\";
};
