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
#include <shellapi.h>
#include "Version.h"
#include "PackageFiles.h"
#include "Utils.h"
#include "../Shared/CommonSharedConstants.h"

// 将当前目录设为程序所在目录
static void SetWorkingDir() noexcept {
	std::wstring path;
	FAIL_FAST_IF_FAILED(wil::GetModuleFileNameW(NULL, path));

	FAIL_FAST_IF_FAILED(PathCchRemoveFileSpec(
		path.data(),
		path.size() + 1
	));

	FAIL_FAST_IF_WIN32_BOOL_FALSE(SetCurrentDirectory(path.c_str()));
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
		return;
	}

	do {
		if (wil::path_is_dot_or_dotdot(findData.cFileName)) {
			continue;
		}
		
		std::wstring curPath = src + L"\\" + findData.cFileName;
		std::wstring destPath = dest + L"\\" + findData.cFileName;
		if (Utils::FileExists(curPath.c_str())) {
			MoveFileEx(curPath.c_str(), destPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
		} else {
			CreateDirectory(destPath.c_str(), nullptr);
			MoveFolder(curPath, destPath);
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

	SetWorkingDir();

	Version oldVersion;
	if (!oldVersion.Parse(Utils::UTF16ToUTF8(lpCmdLine))) {
		return 1;
	}

	std::optional<PackageFiles> oldFiles = PackageFiles::Get(oldVersion);
	if (!oldFiles) {
		// 未找到此版本
		return 1;
	}

	// 检查 Updater.exe 所处环境
	if (!Utils::FileExists(L"Magpie.exe") || !Utils::FileExists(L"update\\Magpie.exe")) {
		return 1;
	}
	
	// 等待 Magpie.exe 退出
	if (!WaitForMagpieToExit()) {
		return 1;
	}

	// 删除旧版本文件，静默地失败
	for (const wchar_t* fileName : oldFiles->files) {
		DeleteFile(fileName);
	}
	for (const wchar_t* folder : oldFiles->folders) {
		RemoveDirectory(folder);
	}

	// 移动新版本
	MoveFolder(L"update", L".");
	
	// 删除 update 文件夹
	wil::RemoveDirectoryRecursiveNoThrow(L"update");

	// 启动 Magpie
	SHELLEXECUTEINFO execInfo{
		.cbSize = sizeof(execInfo),
		.fMask = SEE_MASK_NOASYNC,
		.lpVerb = L"open",
		.lpFile = L"Magpie.exe",
		.nShow = SW_SHOWNORMAL
	};
	ShellExecuteEx(&execInfo);

	return 0;
}
