#pragma once

struct CommonSharedConstants {
	static constexpr const wchar_t* SINGLE_INSTANCE_MUTEX_NAME = L"{4C416227-4A30-4A2F-8F23-8701544DD7D6}";
	static constexpr const wchar_t* TOUCH_HELPER_SINGLE_INSTANCE_MUTEX_NAME = L"{BD7A3F98-F4A9-44B6-9C8A-17B8DE00FEC3}";

	static constexpr const wchar_t* MAIN_WINDOW_CLASS_NAME = L"Magpie_Main";
	static constexpr const wchar_t* TITLE_BAR_WINDOW_CLASS_NAME = L"Magpie_TitleBar";
	static constexpr const wchar_t* NOTIFY_ICON_WINDOW_CLASS_NAME = L"Magpie_NotifyIcon";
	static constexpr const wchar_t* HOTKEY_WINDOW_CLASS_NAME = L"Magpie_Hotkey";
	static constexpr const wchar_t* SCALING_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";
	static constexpr const wchar_t* SWAP_CHAIN_CHILD_WINDOW_CLASS_NAME = L"Magpie_ScalingSwapChain";
	static constexpr const wchar_t* SCALING_BORDER_HELPER_WINDOW_CLASS_NAME = L"Magpie_ScalingBorderHelper";
	static constexpr const wchar_t* DDF_WINDOW_CLASS_NAME = L"Window_Magpie_C322D752-C866-4630-91F5-32CB242A8930";
	static constexpr const wchar_t* TOUCH_HELPER_WINDOW_CLASS_NAME = L"Magpie_TouchHelper";
	static constexpr const wchar_t* TOUCH_HELPER_HOLE_WINDOW_CLASS_NAME = L"Magpie_TouchHelperHole";
	static constexpr const wchar_t* TOAST_WINDOW_CLASS_NAME = L"Magpie_Toast";

	static constexpr const wchar_t* TOUCH_HELPER_EXE_NAME = L"TouchHelper.exe";
	// TouchHelper 有重要更改则提高版本号
	static constexpr uint32_t TOUCH_HELPER_VERSION = 2;

	static constexpr const char* LOG_PATH = "logs\\magpie.log";
	static constexpr const char* REGISTER_TOUCH_HELPER_LOG_PATH = "logs\\register_touch_helper.log";
	static constexpr const wchar_t* CONFIG_DIR = L"config\\";
	static constexpr const wchar_t* CONFIG_FILENAME = L"config.json";
	static constexpr const wchar_t* SOURCES_DIR = L"sources\\";
	static constexpr const wchar_t* EFFECTS_DIR = L"effects\\";
	static constexpr const wchar_t* ASSETS_DIR = L"assets\\";
	static constexpr const wchar_t* CACHE_DIR = L"cache\\";
	static constexpr const wchar_t* UPDATE_DIR = L"update\\";

	static constexpr const wchar_t* OPTION_LAUNCH_WITHOUT_WINDOW = L"-t";

	static constexpr UINT WM_NOTIFY_ICON = WM_USER;

	static constexpr const wchar_t* WM_MAGPIE_SHOWME = L"WM_MAGPIE_SHOWME";
	static constexpr const wchar_t* WM_MAGPIE_QUIT = L"WM_MAGPIE_QUIT";
	static constexpr const wchar_t* WM_MAGPIE_SCALINGCHANGED = L"MagpieScalingChanged";
	static constexpr const wchar_t* WM_MAGPIE_TOUCHHELPER = L"MagpieTouchHelper";

	static constexpr const wchar_t* APP_RESOURCE_MAP_ID = L"Magpie/Resources";
};
