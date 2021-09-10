// DllMain.cpp : Runtime.dll 的入口点。
//


#include "pch.h"
#include "App.h"


HINSTANCE hInst = NULL;
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
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}


void InitLogger() {
	logger = spdlog::rotating_logger_mt(".", "logs/Runtime.log", 100000, 2);
	logger->set_level(spdlog::level::info);
	logger->set_pattern("%Y-%m-%d %H:%M:%S.%e|%l|%s:%!|%v");
	logger->flush_on(spdlog::level::warn);
	spdlog::flush_every(std::chrono::seconds(5));
}

API_DECLSPEC void WINAPI Run(
	void reportStatus(int status, const wchar_t* errorMsgId),
	HWND hwndSrc,
	bool adjustCursorSpeed
) {
	reportStatus(1, nullptr);

	if (!logger) {
		try {
			InitLogger();
		} catch (const spdlog::spdlog_ex&) {
			// 初始化日志失败，直接退出
			reportStatus(0, ErrorMessages::INIT_LOGGER);
			return;
		}
	}

	App& app = App::GetInstance();
	if (!app.Initialize(hInst, hwndSrc, adjustCursorSpeed)) {
		// 初始化失败
		SPDLOG_LOGGER_INFO(logger, "App 初始化失败，返回 GENREIC 消息");
		reportStatus(0, App::GetErrorMsg());
		return;
	}

	SPDLOG_LOGGER_INFO(logger, "汇报初始化完成");
	reportStatus(2, nullptr);

	app.Run();

	SPDLOG_LOGGER_INFO(logger, "汇报已退出");
	reportStatus(0, nullptr);
}
