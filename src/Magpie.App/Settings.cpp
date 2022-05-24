#include "pch.h"
#include "Settings.h"
#if __has_include("Settings.g.cpp")
#include "Settings.g.cpp"
#endif

#include "Utils.h"
#include "StrUtils.h"
#include "Logger.h"

using namespace winrt;
using namespace Windows::Foundation;


static std::wstring GetConfigPath(bool isPortableMode) {
	static constexpr const wchar_t* CONFIG_FILE_PATH = L"config\\config.json";

	if (isPortableMode) {
		return CONFIG_FILE_PATH;
	}

	wchar_t localAppDataDir[MAX_PATH];
	HRESULT hr = SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataDir);
	if (FAILED(hr)) {
		Logger::Get().ComError("SHGetFolderPath 失败", hr);
		return L"";
	}

	return StrUtils::ConcatW(localAppDataDir, L"\\Magpie\\", MAGPIE_VERSION_W, L"\\", CONFIG_FILE_PATH);
}


namespace winrt::Magpie::App::implementation {

bool Settings::Initialize() {
	// 若程序所在目录存在配置文件则为便携模式
	std::wstring configPath = GetConfigPath(true);
	_isPortableMode = Utils::FileExists(configPath.c_str());

	std::string configText;
	if (_isPortableMode) {
		if (!Utils::ReadTextFile(configPath.c_str(), configText)) {
			Logger::Get().Error("读取配置文件失败");
			return false;
		}
	} else {
		configPath = GetConfigPath(false);

		if (!Utils::FileExists(configPath.c_str())) {
			Logger::Get().Info("不存在配置文件");
			return true;
		}

		if (!Utils::ReadTextFile(configPath.c_str(), configText)) {
			Logger::Get().Error("读取配置文件失败");
			return false;
		}
	}

	return true;
}

void Settings::Save() {
	
}

}
