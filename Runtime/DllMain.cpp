// DllMain.cpp : Runtime.dll 的入口点。
//


#include "pch.h"
#include "App.h"


static HINSTANCE hInst = NULL;
std::shared_ptr<spdlog::logger> logger = nullptr;

// DLL 入口
BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		hInst = hModule;
		break;
	case DLL_PROCESS_DETACH:
		App::GetInstance().~App();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

API_DECLSPEC BOOL WINAPI Initialize() {
	try {
		logger = spdlog::rotating_logger_mt(".", "logs/Runtime.log", 100000, 1);
		logger->set_level(spdlog::level::info);
		logger->set_pattern("%Y-%m-%d %H:%M:%S.%e|%l|%s:%!|%v");
		logger->flush_on(spdlog::level::warn);
		spdlog::flush_every(std::chrono::seconds(5));
	} catch (const spdlog::spdlog_ex&) {
		// 初始化日志失败
		return FALSE;
	}

	// 初始化 App
	App& app = App::GetInstance();
	if (!app.Initialize(hInst)) {
		return FALSE;
	}

	return TRUE;
}


API_DECLSPEC const char* WINAPI Run(
	HWND hwndSrc,
	int captureMode,
	bool adjustCursorSpeed,
	bool showFPS,
	bool noVsync
) {
	App& app = App::GetInstance();
	if (!app.Run(hwndSrc, captureMode, adjustCursorSpeed, showFPS, noVsync)) {
		// 初始化失败
		SPDLOG_LOGGER_INFO(logger, "App.Run 失败");
		return app.GetErrorMsg();
	}

	SPDLOG_LOGGER_INFO(logger, "即将退出");
	return nullptr;
}
