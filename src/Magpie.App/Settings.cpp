#include "pch.h"
#include "Settings.h"
#if __has_include("Settings.g.cpp")
#include "Settings.g.cpp"
#endif

#include "Utils.h"
#include "StrUtils.h"
#include "Logger.h"

static constexpr const wchar_t* CONFIG_FILE_PATH = L"config\\config.json";

namespace winrt::Magpie::App::implementation {

bool Settings::Initialize() {
	// 若程序所在目录存在配置文件则为编写模式
	_isPortableMode = Utils::FileExists(CONFIG_FILE_PATH);

	std::string configText;
	if (_isPortableMode) {
		if (!Utils::ReadTextFile(CONFIG_FILE_PATH, configText)) {
			Logger::Get().Error("读取配置文件失败");
			return false;
		}
	} else {
		wchar_t localAppDataDir[MAX_PATH];
		HRESULT hr = SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataDir);
		if (FAILED(hr)) {
			Logger::Get().ComError("SHGetFolderPath 失败", hr);
			return false;
		}

		std::wstring configPath = StrUtils::ConcatW(
			localAppDataDir, L"\\Magpie\\", MAGPIE_VERSION_W, L"\\", CONFIG_FILE_PATH);
		if (!Utils::FileExists(configPath.c_str())) {
			Logger::Get().Info("不存在配置文件");
			return true;
		}

		if (!Utils::ReadTextFile(localAppDataDir, configText)) {
			Logger::Get().Error("读取配置文件失败");
			return false;
		}
	}

	return true;
}

}
