// DllMain.cpp : Runtime.dll 的入口点。
//


#include "pch.h"
#include "App.h"
#include "Utils.h"


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

// 日志级别：
// 0：TRACE，1：DEBUG，...，6：OFF
API_DECLSPEC void WINAPI SetLogLevel(int logLevel) {
	assert(logger);

	logger->flush();
	logger->set_level((spdlog::level::level_enum)logLevel);
	static const char* LOG_LEVELS[7] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL", "OFF" };
	SPDLOG_LOGGER_INFO(logger, fmt::format("当前日志级别：{}", LOG_LEVELS[logLevel]));
}


API_DECLSPEC BOOL WINAPI Initialize(int logLevel) {
	// 初始化日志
	try {
		logger = spdlog::rotating_logger_mt(".", "logs/Runtime.log", 100000, 1);
		logger->set_level((spdlog::level::level_enum)logLevel);
		logger->set_pattern("%Y-%m-%d %H:%M:%S.%e|%l|%s:%!|%v");
		logger->flush_on(spdlog::level::warn);
		spdlog::flush_every(std::chrono::seconds(5));
	} catch (const spdlog::spdlog_ex&) {
		return FALSE;
	}

	SetLogLevel(logLevel);

	// 初始化 App
	if (!App::GetInstance().Initialize(hInst)) {
		return FALSE;
	}

	// 初始化 Hasher
	if (!Utils::Hasher::GetInstance().Initialize()) {
		return FALSE;
	}

	return TRUE;
}

API_DECLSPEC const char* WINAPI Run(
	HWND hwndSrc,
	const char* effectsJson,
	int captureMode,
	bool adjustCursorSpeed,
	bool showFPS,
	bool disableRoundCorner,
	int frameRate	// 0：垂直同步，负数：不限帧率，正数：限制的帧率
) {
	SPDLOG_LOGGER_INFO(logger, fmt::format("运行时参数：\n\thwndSrc：{}\n\tcaptureMode：{}\n\tadjustCursorSpeed：{}\n\tshowFPS：{}\n\tdisableRoundCorner：{}\n\tframeRate：{}", (void*)hwndSrc, captureMode, adjustCursorSpeed, showFPS, disableRoundCorner, frameRate));

	const auto& version = Utils::GetOSVersion();
	SPDLOG_LOGGER_INFO(logger, fmt::format("OS 版本：{}.{}.{}",
		version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber));

	App& app = App::GetInstance();
	if (!app.Run(hwndSrc, effectsJson, captureMode, adjustCursorSpeed, showFPS, disableRoundCorner, frameRate)) {
		// 初始化失败
		SPDLOG_LOGGER_INFO(logger, "App.Run 失败");
		return app.GetErrorMsg();
	}

	SPDLOG_LOGGER_INFO(logger, "即将退出");
	logger->flush();

	return nullptr;
}
