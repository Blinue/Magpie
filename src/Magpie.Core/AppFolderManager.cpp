#include "pch.h"
#include "AppFolderManager.h"
#include "CommonSharedConstants.h"
#include "Win32Helper.h"
#include <ShlObj.h>

#define APP_DIR L"app"
#define DATA_DIR L"data"

namespace Magpie {

bool AppFolderManager::Initialize() noexcept {
	_exeDir = Win32Helper::GetExePath().parent_path();
	if (_exeDir.empty()) {
		return false;
	}

	// dll 搜索路径中添加 app 文件夹以及排除当前目录
	if (!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)) {
		return false;
	}

	if (!AddDllDirectory((_exeDir / APP_DIR).c_str())) {
		return false;
	}

	// 若程序所在目录存在配置文件则为便携模式
	_isPortableMode = Win32Helper::FileExists(StrHelper::Concat(
		_exeDir.native(), L"\\" DATA_DIR L"\\config\\", CommonSharedConstants::CONFIG_FILENAME).c_str());

	// 旧版本便携模式配置文件位置
	_isPortableMode = _isPortableMode || Win32Helper::FileExists(StrHelper::Concat(
		_exeDir.native(), L"\\config\\", CommonSharedConstants::CONFIG_FILENAME).c_str());

	if (_isPortableMode) {
		_workingDir = _exeDir / DATA_DIR;
	} else {
		wil::unique_cotaskmem_string localAppDataDir;
		HRESULT hr = SHGetKnownFolderPath(
			FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, localAppDataDir.put());
		if (FAILED(hr)) {
			return false;
		}

		_workingDir = StrHelper::Concat(localAppDataDir.get(), L"\\Magpie\\" DATA_DIR);
	}

	Win32Helper::CreateDir(_workingDir.c_str());

	if (!SetCurrentDirectory(_workingDir.c_str())) {
		return false;
	}

	return true;
}

std::filesystem::path AppFolderManager::GetAppDir() const noexcept {
	return _exeDir / APP_DIR;
}

const wchar_t* AppFolderManager::GetLogsDir() const noexcept {
	return L"logs";
}

const wchar_t* AppFolderManager::GetSourcesDir() const noexcept {
	return L"sources";
}

const wchar_t* AppFolderManager::GetCacheDir() const noexcept {
	return L"cache";
}

const wchar_t* AppFolderManager::GetConfigDir() const noexcept {
	return L"config";
}

std::filesystem::path AppFolderManager::GetBuiltInShaderEffectsDir() const noexcept {
	return _exeDir / APP_DIR L"\\effects\\shaders";
}

std::filesystem::path AppFolderManager::GetD3D12Dir() const noexcept {
	return _exeDir / APP_DIR L"\\D3D12";
}

std::filesystem::path AppFolderManager::GetUpdateDir() const noexcept {
	// 位于根目录中，非打包应用更新时才会使用
	return _exeDir / CommonSharedConstants::UPDATE_DIR;
}

}
