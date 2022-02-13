// Copyright (c) 2021 - present, Liu Xu
//
//  This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "pch.h"
#include "App.h"
#include "Utils.h"
#include "StrUtils.h"


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
API_DECLSPEC void WINAPI SetLogLevel(UINT logLevel) {
	assert(logger);

	logger->flush();
	logger->set_level((spdlog::level::level_enum)logLevel);
	static const char* LOG_LEVELS[7] = {
		"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL", "OFF"
	};
	SPDLOG_LOGGER_INFO(logger, fmt::format("当前日志级别：{}", LOG_LEVELS[logLevel]));
}


API_DECLSPEC BOOL WINAPI Initialize(
	UINT logLevel,
	const char* logFileName,
	int logArchiveAboveSize,
	int logMaxArchiveFiles
) {
	// 初始化日志
	try {
		logger = spdlog::rotating_logger_mt(".", logFileName, logArchiveAboveSize, logMaxArchiveFiles);
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
	UINT flags,
	UINT captureMode,
	float cursorZoomFactor,	// 负数和 0：和源原窗口相同，正数：缩放比例
	UINT cursorInterpolationMode,	// 0：最近邻，1：双线性
	int adapterIdx,
	UINT multiMonitorUsage,	// 0：最近 1：相交 2：所有
	UINT cropLeft,
	UINT cropTop,
	UINT cropRight,
	UINT cropBottom
) {
	if (!hwndSrc || !IsWindow(hwndSrc)) {
		SPDLOG_LOGGER_CRITICAL(logger, "非法的源窗口句柄");
		return ErrorMessages::GENERIC;
	}

	const auto& version = Utils::GetOSVersion();
	SPDLOG_LOGGER_INFO(logger, fmt::format("OS 版本：{}.{}.{}",
		version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber));

	int len = GetWindowTextLength(hwndSrc);
	if (len <= 0) {
		SPDLOG_LOGGER_INFO(logger, "源窗口无标题");
	} else {
		std::wstring title(len, 0);
		if (!GetWindowText(hwndSrc, &title[0], int(title.size() + 1))) {
			SPDLOG_LOGGER_ERROR(logger, "获取源窗口标题失败");
		} else {
			SPDLOG_LOGGER_INFO(logger, "源窗口标题：" + StrUtils::UTF16ToUTF8(title));
		}
	}

	App& app = App::GetInstance();
	if (!app.Run(hwndSrc, effectsJson, captureMode,
		cursorZoomFactor, cursorInterpolationMode, adapterIdx, multiMonitorUsage,
		RECT{(LONG)cropLeft, (LONG)cropTop, (LONG)cropRight, (LONG)cropBottom}, flags)
	) {
		// 初始化失败
		SPDLOG_LOGGER_INFO(logger, "App.Run 失败");
		return app.GetErrorMsg();
	}

	SPDLOG_LOGGER_INFO(logger, "即将退出");
	logger->flush();

	return nullptr;
}


// ----------------------------------------------------------------------------------------
// 以下函数在用户界面的主线程上调用



API_DECLSPEC const char* WINAPI GetAllGraphicsAdapters(const char* delimiter) {
	static std::string result;
	result.clear();

	ComPtr<IDXGIFactory1> dxgiFactory;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		return "";
	}

	ComPtr<IDXGIAdapter1> adapter;
	for (UINT adapterIndex = 0;
			SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex,
				adapter.ReleaseAndGetAddressOf()));
			adapterIndex++
	) {
		if (adapterIndex > 0) {
			result += delimiter;
		}

		DXGI_ADAPTER_DESC1 desc;
		HRESULT hr = adapter->GetDesc1(&desc);
		if (SUCCEEDED(hr)) {
			result += StrUtils::UTF16ToUTF8(desc.Description);
		} else {
			result += "???";
		}
	}

	return result.c_str();
}
