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

	std::filesystem::path GetBuiltInShaderEffectsDir() const noexcept;

	std::filesystem::path GetD3D12Dir() const noexcept;

	wil::zwstring_view GetLogsDir() const noexcept;

	wil::zwstring_view GetSourcesDir() const noexcept;

	wil::zwstring_view GetCacheDir() const noexcept;

	wil::zwstring_view GetConfigDir() const noexcept;

	wil::zwstring_view GetExtDir() const noexcept;

	wil::zwstring_view GetTempDir() const noexcept;

private:
	AppFolderManager() = default;

	std::filesystem::path _exeDir;
	std::filesystem::path _workingDir;
	bool _isPortableMode = false;
};

}
