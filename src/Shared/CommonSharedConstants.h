#pragma once
#include "CommonPCH.h"


struct CommonSharedConstants {
	static constexpr const COLORREF LIGHT_TINT_COLOR = RGB(243, 243, 243);
	static constexpr const COLORREF DARK_TINT_COLOR = RGB(32, 32, 32);

	static constexpr const char* LOG_PATH = "logs\\magpie.log";
	static constexpr const char* CONFIG_PATH = "config\\config.json";
	static constexpr const wchar_t* CONFIG_PATH_W = L"config\\config.json";
};
