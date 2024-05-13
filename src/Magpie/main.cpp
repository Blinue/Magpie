// Copyright (c) 2021 - present, Liu Xu
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
#include "XamlApp.h"
#include "Win32Utils.h"
#include "TouchHelper.h"
#include "CommonSharedConstants.h"
#include "StrUtils.h"

// 将当前目录设为程序所在目录
static std::wstring SetWorkingDir() noexcept {
	std::wstring path = Win32Utils::GetExePath();

	FAIL_FAST_IF_FAILED(PathCchRemoveFileSpec(
		path.data(),
		path.size() + 1
	));

	FAIL_FAST_IF_WIN32_BOOL_FALSE(SetCurrentDirectory(path.c_str()));

	path.resize(StrUtils::StrLen(path.c_str()));
	return path;
}

static void InitializeLogger(const char* logFilePath) noexcept {
	Logger::Get().Initialize(
		spdlog::level::info,
		logFilePath,
		100000,
		2
	);
}

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ wchar_t* lpCmdLine,
	_In_ int /*nCmdShow*/
) {
#ifdef _DEBUG
	SetThreadDescription(GetCurrentThread(), L"Magpie 主线程");
#endif

	// 堆损坏时终止进程
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, nullptr, 0);

	std::wstring workingDir = SetWorkingDir();

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

	Logger::Get().Info(fmt::format("程序启动\n\t版本: {}\n\t管理员: {}",
#ifdef MAGPIE_VERSION_TAG
		STRING(MAGPIE_VERSION_TAG),
#else
		"dev",
#endif
		Win32Utils::IsProcessElevated() ? "是" : "否"
	));

	if (mode == RegisterTouchHelper) {
		// 使 TouchHelper 获得 UIAccess 权限
		return Magpie::TouchHelper::Register() ? 0 : 1;
	} else if (mode == UnRegisterTouchHelper) {
		return Magpie::TouchHelper::Unregister() ? 0 : 1;
	}

	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	workingDir += L"\\third_party";
	AddDllDirectory(workingDir.c_str());

	auto& app = Magpie::XamlApp::Get();
	if (!app.Initialize(hInstance, lpCmdLine)) {
		return 0;
	}

	return app.Run();
}
