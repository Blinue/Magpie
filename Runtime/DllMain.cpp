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
#include "Logger.h"


#define API_DECLSPEC extern "C" __declspec(dllexport)

static HINSTANCE hInst = NULL;


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

// 日志级别：
// 0：TRACE，1：DEBUG，...，6：OFF
API_DECLSPEC void WINAPI SetLogLevel(UINT logLevel) {
	Logger::Get().SetLevel((spdlog::level::level_enum)logLevel);
}


API_DECLSPEC BOOL WINAPI Initialize(
	UINT logLevel,
	const char* logFileName,
	int logArchiveAboveSize,
	int logMaxArchiveFiles
) {
	// 初始化日志
	if (!Logger::Get().Initialize(logLevel, logFileName, logArchiveAboveSize, logMaxArchiveFiles)) {
		return FALSE;
	}

	// 初始化 App
	if (!App::Get().Initialize(hInst)) {
		return FALSE;
	}

	// 初始化 Hasher
	if (!Utils::Hasher::Get().Initialize()) {
		return FALSE;
	}

	return TRUE;
}

API_DECLSPEC const char* WINAPI Run(
	HWND hwndSrc,
	const char* effectsJson,
	UINT flags,
	UINT captureMode,
	float cursorZoomFactor,	// 负数和 0：和源窗口相同，正数：缩放比例
	UINT cursorInterpolationMode,	// 0：最近邻，1：双线性
	int adapterIdx,
	UINT multiMonitorUsage,	// 0：最近 1：相交 2：所有
	UINT cropLeft,
	UINT cropTop,
	UINT cropRight,
	UINT cropBottom
) {
	Logger& logger = Logger::Get();

	if (!hwndSrc || !IsWindow(hwndSrc)) {
		logger.Critical("非法的源窗口句柄");
		return ErrorMessages::GENERIC;
	}

	const auto& version = Utils::GetOSVersion();
	logger.Info(fmt::format("OS 版本：{}.{}.{}",
		version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber));

	int len = GetWindowTextLength(hwndSrc);
	if (len <= 0) {
		logger.Info("源窗口无标题");
	} else {
		std::wstring title(len, 0);
		if (!GetWindowText(hwndSrc, &title[0], int(title.size() + 1))) {
			Logger::Get().Error("获取源窗口标题失败");
		} else {
			Logger::Get().Info(StrUtils::Concat("源窗口标题：", StrUtils::UTF16ToUTF8(title)));
		}
	}

	App& app = App::Get();
	if (!app.Run(hwndSrc, effectsJson, captureMode,
		cursorZoomFactor, cursorInterpolationMode, adapterIdx, multiMonitorUsage,
		RECT{(LONG)cropLeft, (LONG)cropTop, (LONG)cropRight, (LONG)cropBottom}, flags)
	) {
		// 初始化失败
		Logger::Get().Info("App.Run 失败");
		return app.GetErrorMsg();
	}

	logger.Info("即将退出");
	logger.Flush();

	return nullptr;
}


// ----------------------------------------------------------------------------------------
// 以下函数在用户界面的主线程上调用



API_DECLSPEC const char* WINAPI GetAllGraphicsAdapters(const char* delimiter) {
	static std::string result;
	result.clear();

	winrt::com_ptr<IDXGIFactory1> dxgiFactory;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		return "";
	}

	winrt::com_ptr<IDXGIAdapter1> adapter;
	for (UINT adapterIndex = 0;
		SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex,adapter.put()));
		++adapterIndex
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
