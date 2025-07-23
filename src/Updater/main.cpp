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
#include "CommonSharedConstants.h"
#include "Logger.h"
#include "PackageFiles.h"
#include "StrHelper.h"
#include "Version.h"
#include <shellapi.h>

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

	logPath.append(CommonSharedConstants::UPDATER_LOG_NAME);

	Logger::Get().Initialize(
		spdlog::level::info,
		std::move(logPath),
		CommonSharedConstants::LOG_MAX_SIZE,
		1
	);
}

// 将当前目录设为程序所在目录
static bool SetWorkingDir() noexcept {
	std::wstring exePath;
	HRESULT hr = wil::GetModuleFileNameW(NULL, exePath);
	if (FAILED(hr)) {
		Logger::Get().ComError("GetModuleFileNameW 失败", hr);
		return false;
	}

	std::filesystem::path parentPath =
		std::filesystem::path(std::move(exePath)).parent_path();

	if (!SetCurrentDirectory(parentPath.c_str())) {
		Logger::Get().Win32Error("SetCurrentDirectory 失败");
		return false;
	}

	return true;
}

static bool FileExists(const wchar_t* fileName) noexcept {
	DWORD attrs = GetFileAttributes(fileName);
	// 排除文件夹
	return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

static bool WaitForMagpieToExit() noexcept {
	{
		wil::unique_mutex_nothrow hSingleInstanceMutex;
		if (hSingleInstanceMutex.try_create(CommonSharedConstants::SINGLE_INSTANCE_MUTEX_NAME)) {
			if (wil::handle_wait(hSingleInstanceMutex.get(), 10000)) {
				hSingleInstanceMutex.ReleaseMutex();
			}
		}
	}

	// 即使 mutex 已被释放，Magpie.exe 仍有可能正在后台执行清理工作
	// 尝试删除 Magpie.exe，直到成功为止
	for (int i = 0; i < 1000; ++i) {
		if (DeleteFile(L"Magpie.exe")) {
			return true;
		}

		Sleep(10);
	}

	// 超时
	return false;
}

static void MoveFolder(const std::wstring& src, const std::wstring& dest) noexcept {
	WIN32_FIND_DATA findData{};
	wil::unique_hfind hFind(FindFirstFileEx((std::wstring(src) + L"\\*").c_str(),
		FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
	if (!hFind) {
		Logger::Get().Win32Error("FindFirstFileEx 失败");
		return;
	}

	do {
		if (wil::path_is_dot_or_dotdot(findData.cFileName)) {
			continue;
		}
		
		std::wstring curPath = src + L"\\" + findData.cFileName;
		std::wstring destPath = dest + L"\\" + findData.cFileName;
		if (FileExists(curPath.c_str())) {
			if (MoveFileEx(curPath.c_str(), destPath.c_str(),
				MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
			{
				Logger::Get().Info(StrHelper::Concat("已移动 ", StrHelper::UTF16ToUTF8(destPath)));
			} else {
				Logger::Get().Win32Error(StrHelper::Concat(
					"移动 ", StrHelper::UTF16ToUTF8(destPath), " 失败"));
			}
		} else {
			if (CreateDirectory(destPath.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS) {
				Logger::Get().Info(StrHelper::Concat("已创建文件夹 ", StrHelper::UTF16ToUTF8(destPath)));
				MoveFolder(curPath, destPath);
			} else {
				Logger::Get().Win32Error(StrHelper::Concat(
					"创建文件夹 ", StrHelper::UTF16ToUTF8(destPath), " 失败"));
			}
		}
	} while (FindNextFile(hFind.get(), &findData));
}

int APIENTRY wWinMain(
	_In_ HINSTANCE /*hInstance*/,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ wchar_t* lpCmdLine,
	_In_ int /*nCmdShow*/)
{
	if (!lpCmdLine || lpCmdLine[0] == 0) {
		// 参数为空则什么事都不做，以防止用户手动启动 Updater.exe
		return 0;
	}

	InitializeLogger();

	Logger::Get().Info("程序启动");

	if (!SetWorkingDir()) {
		Logger::Get().Critical("SetWorkingDir 失败");
		return 1;
	}

	Version oldVersion;
	if (!oldVersion.Parse(StrHelper::UTF16ToUTF8(lpCmdLine))) {
		Logger::Get().Critical(StrHelper::Concat(
			"解析版本号失败: ", StrHelper::UTF16ToUTF8(lpCmdLine)));
		return 1;
	}

	Logger::Get().Info(StrHelper::Concat("旧版本号: ", oldVersion.ToString<char>()));

	std::optional<PackageFiles> oldFiles = PackageFiles::Get(oldVersion);
	if (!oldFiles) {
		Logger::Get().Critical(StrHelper::Concat("未找到版本 ", oldVersion.ToString<char>()));
		return 1;
	}

	// 检查 Updater.exe 所处环境
	if (!FileExists(L"Magpie.exe") || !FileExists(L"update\\Magpie.exe")) {
		Logger::Get().Critical("未找到 Magpie.exe 或 update\\Magpie.exe");
		return 1;
	}
	
	// 等待 Magpie.exe 退出
	if (!WaitForMagpieToExit()) {
		Logger::Get().Critical("等待 Magpie.exe 退出超时");
		return 1;
	}

	// 删除旧版本文件
	for (const wchar_t* fileName : oldFiles->files) {
		if (DeleteFile(fileName)) {
			Logger::Get().Info(StrHelper::Concat(
				"已删除文件 ", StrHelper::UTF16ToUTF8(fileName)));
		} else {
			Logger::Get().Win32Error(StrHelper::Concat(
				"删除文件 ", StrHelper::UTF16ToUTF8(fileName), " 失败"));
		}
	}
	for (const wchar_t* folder : oldFiles->folders) {
		if (RemoveDirectory(folder)) {
			Logger::Get().Info(StrHelper::Concat(
				"已删除文件夹 ", StrHelper::UTF16ToUTF8(folder)));
		} else {
			Logger::Get().Win32Error(StrHelper::Concat(
				"删除文件夹 ", StrHelper::UTF16ToUTF8(folder), " 失败"));
		}
	}

	// 移动新版本
	MoveFolder(CommonSharedConstants::UPDATE_DIR, L".");
	
	// 删除 update 文件夹
	HRESULT hr = wil::RemoveDirectoryRecursiveNoThrow(CommonSharedConstants::UPDATE_DIR);
	if (SUCCEEDED(hr)) {
		Logger::Get().Info("已删除 update 文件夹");
	} else {
		Logger::Get().ComError("RemoveDirectoryRecursiveNoThrow 失败", hr);
	}

	// 启动 Magpie
	SHELLEXECUTEINFO execInfo{
		.cbSize = sizeof(execInfo),
		.fMask = SEE_MASK_NOASYNC,
		.lpVerb = L"open",
		.lpFile = L"Magpie.exe",
		.nShow = SW_SHOWNORMAL
	};
	if (ShellExecuteEx(&execInfo)) {
		Logger::Get().Info("已启动 Magpie.exe");
	} else {
		Logger::Get().Win32Error("ShellExecuteEx 失败");
	}

	Logger::Get().Info("程序退出");
	Logger::Get().Flush();
	return 0;
}
