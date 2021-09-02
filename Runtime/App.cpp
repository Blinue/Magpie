#include "pch.h"
#include "App.h"



bool App::Initialize(std::shared_ptr<spdlog::logger> logger) {
	_logger = logger;

	SPDLOG_LOGGER_INFO(logger, "正在初始化 App");

	// 确保只初始化 COM 一次
	static bool initalized = false;
	if (!initalized) {
		HRESULT hr = Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, fmt::format("初始化 COM 失败\n\tHRESULT：{}", hr));
			return false;
		}
		SPDLOG_LOGGER_INFO(logger, "初始化 COM 成功");

		initalized = true;
	}
	
	return true;
}

