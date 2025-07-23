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
#include "App.h"
#include "CommonSharedConstants.h"
#include "Logger.h"

static void InitializeLogger() noexcept {
	// 日志文件创建在 Temp 目录中
	std::wstring logPath(MAX_PATH + 1, L'\0');
	const DWORD len = GetTempPath(MAX_PATH + 2, logPath.data());
	if (len <= 0) {
		return;
	}

	logPath.resize(len);
	if (!logPath.ends_with(L'\\')) {
		logPath.push_back(L'\\');
	}

	logPath.append(CommonSharedConstants::TOUCH_HELPER_LOG_NAME);

	Logger::Get().Initialize(
		spdlog::level::info,
		std::move(logPath),
		CommonSharedConstants::LOG_MAX_SIZE,
		1
	);
}

int APIENTRY wWinMain(
	_In_ HINSTANCE /*hInstance*/,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ LPWSTR /*lpCmdLine*/,
	_In_ int /*nCmdShow*/
) {
	InitializeLogger();

	Logger::Get().Info("程序启动");

	App& app = App::Get();
	if (!app.Initialzie()) {
		Logger::Get().Critical("初始化失败");
		return 0;
	}

	const int ret = app.Run();
	Logger::Get().Info("程序退出");
	Logger::Get().Flush();
	return ret;
}
