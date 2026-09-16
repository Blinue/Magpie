#pragma once
#include "Singleton.h"

namespace Magpie {

// 文件结构见 https://github.com/Blinue/Magpie/pull/1348#issuecomment-4241246636
class AppFolderManager : public Singleton<AppFolderManager> {
	friend Singleton<AppFolderManager>;

public:
	bool Initialize() noexcept;

	bool IsPortableMode() const noexcept {
		return _isPortableMode;
	}

	const std::filesystem::path& GetExeDir() const noexcept {
		return _exeDir;
	}

	const std::filesystem::path& GetWorkingDir() const noexcept {
		return _workingDir;
	}

	std::filesystem::path GetAppDir() const noexcept;

	const wchar_t* GetLogsDir() const noexcept;

	const wchar_t* GetSourcesDir() const noexcept;

	const wchar_t* GetCacheDir() const noexcept;

	const wchar_t* GetConfigDir() const noexcept;

	std::filesystem::path GetBuiltInShaderEffectsDir() const noexcept;

	std::filesystem::path GetD3D12Dir() const noexcept;

	std::filesystem::path GetUpdateDir() const noexcept;

private:
	AppFolderManager() = default;

	std::filesystem::path _exeDir;
	std::filesystem::path _workingDir;
	bool _isPortableMode = false;
};

}
