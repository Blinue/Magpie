// Copyright (c) Xu
//
// This program is free software: you can redistribute it and/or modify
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
#include "OnnxStatus.h"
#include "StrHelper.h"
#include "App.h"
#include "Win32Helper.h"
#include "TouchHelper.h"
#include "CommonSharedConstants.h"
#include "Logger.h"

using namespace Magpie;
using namespace winrt::Magpie::implementation;

// 将当前目录设为程序所在目录
static void SetWorkingDir() noexcept {
	FAIL_FAST_IF_WIN32_BOOL_FALSE(SetCurrentDirectory(
		Win32Helper::GetExePath().parent_path().c_str()));
}

// 固定 third_party 中的运行库，必须在使用 ORT 之前、日志初始化之后调用
// Pin the runtimes shipped in third_party\. Must run before anything touches
// ONNX Runtime, and after the logger exists so failures can be reported.
//
// Windows ships its own ONNX Runtime in System32, and
// SetDefaultDllDirectories searches System32 before any AddDllDirectory
// path - so an unqualified load binds the OS copy. Built against newer
// headers, GetApi(ORT_API_VERSION) then returns nullptr and the first Ort
// call dereferences it. Loading by absolute path pins ours; later
// resolutions of the same name reuse the loaded module.
//
// This matters only since v0.12: onnxruntime.lib used to link into
// Magpie.App.dll, which loaded after the search path was extended.
static void PinThirdPartyRuntimes() noexcept {
#ifndef MAGPIE_ONNX_ENABLED
	// ONNX 只在 x64 上编译 / the feature is compiled out on other platforms
	return;
#else
	const std::filesystem::path exeDir = Win32Helper::GetExePath().parent_path();
	const std::wstring thirdPartyDir = (exeDir / L"third_party").wstring();

	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	AddDllDirectory(thirdPartyDir.c_str());

	bool ortLoaded = false;
	for (const wchar_t* dllName : { L"onnxruntime.dll", L"DirectML.dll" }) {
		const std::wstring dllPath = thirdPartyDir + L"\\" + dllName;
		if (LoadLibraryEx(dllPath.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
			LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR)) {
			if (dllName == L"onnxruntime.dll"sv) {
				ortLoaded = true;
			}
		} else {
			// 不致命：没有 ONNX 时缩放仍可工作
			// Not fatal - scaling still works without ONNX.
			Logger::Get().Win32Error(StrHelper::Concat(
				"加载失败 / failed to preload ", StrHelper::UTF16ToUTF8(dllPath)));
		}
	}

	// ORT_API_MANUAL_INIT 关掉了头文件里 main 之前的静态初始化，
	// 现在才绑定 API 表，确保来自我们刚固定的 DLL
	// ORT_API_MANUAL_INIT disabled the header's pre-main static init; bind the
	// API table now so it comes from the DLL just pinned.
	if (ortLoaded) {
		OnnxStatus::InitOrtApi();
	} else {
		// 绝不能在这里碰 Ort：延迟加载会在加载器内部抛出
		// Never touch Ort here - the delay-load would raise inside the loader.
		Logger::Get().Error(
			"third_party\\onnxruntime.dll 缺失，已禁用 AI 放大 / missing, AI "
			"upscaling disabled for this session");
	}
#endif
}

static void InitializeLogger(const wchar_t* logFilePath) noexcept {
	// 最多两个日志文件，每个最多 500KB
	Logger::Get().Initialize(
		spdlog::level::info,
		logFilePath,
		CommonSharedConstants::LOG_MAX_SIZE,
		1
	);
}

int APIENTRY wWinMain(
	_In_ HINSTANCE /*hInstance*/,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ wchar_t* lpCmdLine,
	_In_ int /*nCmdShow*/
) {
#ifdef _DEBUG
	SetThreadDescription(GetCurrentThread(), L"Magpie-主线程");
#endif
	
	// 堆损坏时终止进程
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, nullptr, 0);

	SetWorkingDir();

	enum {
		Normal,
		RegisterTouchHelper,
		UnRegisterTouchHelper
	} mode = [&]() {
		if (lpCmdLine == L"-r"sv) {
			return RegisterTouchHelper;
		} else if (lpCmdLine == L"-ur"sv) {
			return UnRegisterTouchHelper;
		} else {
			return Normal;
		}
	}();

	InitializeLogger(mode == Normal ?
		CommonSharedConstants::LOG_PATH :
		CommonSharedConstants::REGISTER_TOUCH_HELPER_LOG_PATH);

	PinThirdPartyRuntimes();

	Logger::Get().Info(fmt::format("程序启动\n\t版本: {}\n\tOS 版本: {}\n\t管理员: {}",
#ifdef MP_VERSION_STRING
		STRINGIFY(MP_VERSION_STRING),
#elif defined(MP_COMMIT_ID)
		"dev (" STRINGIFY(MP_COMMIT_ID) ")",
#else
		"dev",
#endif
		Win32Helper::GetOSVersion().ToString<char>(),
		Win32Helper::IsProcessElevated() ? "是" : "否"
	));

	if (mode == RegisterTouchHelper) {
		// 使 TouchHelper 获得 UIAccess 权限
		return Magpie::TouchHelper::Register() ? 0 : 1;
	} else if (mode == UnRegisterTouchHelper) {
		return Magpie::TouchHelper::Unregister() ? 0 : 1;
	}

	// 程序结束时也不应调用 uninit_apartment
	// 见 https://kennykerr.ca/2018/03/24/cppwinrt-hosting-the-windows-runtime/
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	auto& app = App::Get();
	if (!app.Initialize(lpCmdLine)) {
		return 0;
	}

	return app.Run();
}
