#pragma once

struct CommonSharedConstants {
	static constexpr const wchar_t* MAIN_WINDOW_CLASS_NAME = L"Magpie_Main";
	static constexpr const wchar_t* TITLE_BAR_WINDOW_CLASS_NAME = L"Magpie_TitleBar";
	static constexpr const wchar_t* NOTIFY_ICON_WINDOW_CLASS_NAME = L"Magpie_NotifyIcon";
	static constexpr const wchar_t* HOTKEY_WINDOW_CLASS_NAME = L"Magpie_Hotkey";
	static constexpr const wchar_t* SCALING_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";
	static constexpr const wchar_t* DDF_WINDOW_CLASS_NAME = L"Window_Magpie_C322D752-C866-4630-91F5-32CB242A8930";

	static constexpr const COLORREF LIGHT_TINT_COLOR = RGB(243, 243, 243);
	static constexpr const COLORREF DARK_TINT_COLOR = RGB(32, 32, 32);

	static constexpr const char* LOG_PATH = "logs\\magpie.log";
	static constexpr const wchar_t* CONFIG_DIR = L"config\\";
	static constexpr const wchar_t* CONFIG_FILENAME = L"config.json";
	static constexpr const wchar_t* SOURCES_DIR = L"sources\\";
	static constexpr const wchar_t* EFFECTS_DIR = L"effects\\";
	static constexpr const wchar_t* ASSETS_DIR = L"assets\\";
	static constexpr const wchar_t* CACHE_DIR = L"cache\\";
	static constexpr const wchar_t* UPDATE_DIR = L"update\\";

	static constexpr const wchar_t* OPTION_LAUNCH_WITHOUT_WINDOW = L"-t";

#ifndef IDI_APP
	// 来自 Magpie\resource.h
	static constexpr const UINT IDI_APP = 101;
#endif

	static constexpr const UINT WM_NOTIFY_ICON = WM_USER;
	static constexpr const UINT WM_QUIT_MAGPIE = WM_USER + 1;
	static constexpr const UINT WM_RESTART_MAGPIE = WM_USER + 2;

	static constexpr const wchar_t* WM_MAGPIE_SHOWME = L"WM_MAGPIE_SHOWME";
	static constexpr const wchar_t* WM_MAGPIE_QUIT = L"WM_MAGPIE_QUIT";

	static constexpr const wchar_t* APP_RESOURCE_MAP_ID = L"Magpie.App/Resources";
};
