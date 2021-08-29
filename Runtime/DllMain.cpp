// DllMain.cpp : Runtime.dll 的入口点。
//


#include "pch.h"
#include "MagWindow.h"
#include "Env.h"


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

void InitLog() {
	if (logger) {
		return;
	}

	logger = spdlog::rotating_logger_mt(".", "logs/Runtime.log", 100000, 2);
	logger->set_level(spdlog::level::info);
	logger->set_pattern("%Y-%m-%d %H:%M:%S.%e|%l|%s:%!|%v");
	logger->flush_on(spdlog::level::warn);
	spdlog::flush_every(std::chrono::seconds(5));
}

API_DECLSPEC void WINAPI RunMagWindow(
	void reportStatus(int status, const wchar_t* errorMsgId),
	HWND hwndSrc,
	const char* scaleModel,
	int captureMode,
	int bufferPrecision,
	bool showFPS,
	bool adjustCursorSpeed,
	bool noDisturb
) {
	try {
		InitLog();
	} catch (const spdlog::spdlog_ex& e) {
		std::wstring msg;
		Utils::UTF8ToUTF16(e.what(), msg);
		Debug::WriteErrorMessage(fmt::format(L"spdlog初始化失败：{}", msg));
		reportStatus(2, msg.c_str());
		return;
	}

	Debug::ThrowIfComFailed(
		CoInitializeEx(NULL, COINIT_MULTITHREADED),
		L"初始化 COM 出错"
	);

	if (!IsWindow(hwndSrc) || !IsWindowVisible(hwndSrc) || !Utils::GetWindowShowCmd(hwndSrc) == SW_NORMAL) {
		SPDLOG_LOGGER_CRITICAL(logger, "不合法的源窗口");
		reportStatus(0, ErrorMessages::INVALID_SOURCE_WINDOW);
		return;
	}

	try {
		Debug::Assert(
			captureMode >= 0 && captureMode <= 1,
			L"非法的抓取模式"
		);

		Env::CreateInstance(hInst, hwndSrc, scaleModel, captureMode, bufferPrecision, showFPS, adjustCursorSpeed, noDisturb);
		MagWindow::CreateInstance();
	} catch(const magpie_exception& e) {
		reportStatus(0, (L"创建全屏窗口出错：" + e.what()).c_str());
		return;
	} catch (const std::exception& e) {
		SPDLOG_LOGGER_CRITICAL(logger, "创建全屏窗口出错：{}", e.what());
		reportStatus(0, ErrorMessages::GENERIC);
		return;
	}
	
	reportStatus(2, nullptr);

	// 主消息循环
	std::wstring errMsg = MagWindow::RunMsgLoop();

	Env::$instance = nullptr;
	reportStatus(0, errMsg.empty() ? nullptr : errMsg.c_str());
	SPDLOG_LOGGER_INFO(logger, "全屏窗口已退出");
	logger->flush();
}
