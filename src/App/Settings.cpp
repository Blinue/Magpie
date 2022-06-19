#include "pch.h"
#include "Settings.h"
#if __has_include("Settings.g.cpp")
#include "Settings.g.cpp"
#endif

#include "Win32Utils.h"
#include "StrUtils.h"
#include "Logger.h"
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

using namespace winrt;
using namespace Windows::Foundation;


static constexpr const wchar_t* CONFIG_PATH = L"config\\config.json";

static hstring GetWorkingDir(bool isPortableMode) {
	if (isPortableMode) {
		wchar_t curDir[MAX_PATH];
		if (!GetCurrentDirectory(MAX_PATH, curDir)) {
			return L"";
		}

		std::wstring result = curDir;
		if (result.back() != L'\\') {
			result += L'\\';
		}
		return result.c_str();
	} else {
		wchar_t localAppDataDir[MAX_PATH];
		HRESULT hr = SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataDir);
		if (FAILED(hr)) {
			return L"";
		}

		return StrUtils::ConcatW(
			localAppDataDir, 
			localAppDataDir[StrUtils::StrLen(localAppDataDir) - 1] == L'\\' ? L"Magpie\\" : L"\\Magpie\\",
			MAGPIE_VERSION_W,
			L"\\"
		).c_str();
	}
}


namespace winrt::Magpie::App::implementation {

bool Settings::Initialize(uint64_t pLogger) {
	// 若程序所在目录存在配置文件则为便携模式
	_isPortableMode = Win32Utils::FileExists(CONFIG_PATH);
	_workingDir = GetWorkingDir(_isPortableMode);

	// 初始化日志的同时确保 WorkingDir 存在
	Logger& logger = Logger::Get();
	logger.Initialize(
		spdlog::level::info,
		StrUtils::Concat(StrUtils::UTF16ToANSI(_workingDir), "logs\\magpie.log").c_str(),
		100000,
		2
	);

	((Logger*)pLogger)->Initialize(logger);

	logger.Info(StrUtils::Concat("程序启动\n\t便携模式：", IsPortableMode() ? "是" : "否"));

	std::wstring configPath = StrUtils::ConcatW(_workingDir, CONFIG_PATH);

	if (!Win32Utils::FileExists(configPath.c_str())) {
		Logger::Get().Info("不存在配置文件");
		return true;
	}

	std::string configText;
	if (!Win32Utils::ReadTextFile(configPath.c_str(), configText)) {
		Logger::Get().Error("读取配置文件失败");
		return false;
	}

	if (configText.empty()) {
		Logger::Get().Info("配置文件为空");
		return true;
	}

	rapidjson::Document doc;
	if (doc.Parse(configText.c_str(), configText.size()).HasParseError()) {
		Logger::Get().Error(fmt::format("解析配置失败\n\t错误码：{}", doc.GetParseError()));
		return false;
	}

	if (!doc.IsObject()) {
		return false;
	}

	const auto& root = doc.GetObj();
	{
		auto themeNode = root.FindMember("theme");
		if (themeNode != root.MemberEnd()) {
			if (!themeNode->value.IsNumber()) {
				return false;
			}

			_theme = themeNode->value.GetInt();
		}
	}
	{
		auto windowPosNode = root.FindMember("windowPos");
		if (windowPosNode != root.MemberEnd()) {
			if (!windowPosNode->value.IsObject()) {
				return false;
			}

			const auto& windowRectObj = windowPosNode->value.GetObj();

			auto xNode = windowRectObj.FindMember("x");
			if (xNode == windowRectObj.MemberEnd() || !xNode->value.IsNumber()) {
				return false;
			}
			_windowRect.X = (float)xNode->value.GetInt();

			auto yNode = windowRectObj.FindMember("y");
			if (yNode == windowRectObj.MemberEnd() || !yNode->value.IsNumber()) {
				return false;
			}
			_windowRect.Y = (float)yNode->value.GetInt();

			auto widthNode = windowRectObj.FindMember("width");
			if (widthNode == windowRectObj.MemberEnd() || !widthNode->value.IsNumber()) {
				return false;
			}
			_windowRect.Width = (float)widthNode->value.GetInt();

			auto heightNode = windowRectObj.FindMember("height");
			if (heightNode == windowRectObj.MemberEnd() || !heightNode->value.IsNumber()) {
				return false;
			}
			_windowRect.Height = (float)heightNode->value.GetInt();

			auto maximizedNode = windowRectObj.FindMember("maximized");
			if (maximizedNode == windowRectObj.MemberEnd() || !maximizedNode->value.IsBool()) {
				return false;
			}
			_isWindowMaximized = maximizedNode->value.GetBool();
		}
	}

	return true;
}

bool Settings::Save() {
	std::wstring configDir = StrUtils::ConcatW(_workingDir, L"config");
	if (!Win32Utils::CreateDir(configDir)) {
		Logger::Get().Error("创建配置文件夹失败");
		return false;
	}

	rapidjson::StringBuffer json;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(json);
	writer.StartObject();

	writer.Key("theme");
	writer.Int(_theme);

	writer.Key("windowPos");
	writer.StartObject();
	writer.Key("x");
	writer.Int((int)std::lroundf(_windowRect.X));
	writer.Key("y");
	writer.Int((int)std::lroundf(_windowRect.Y));
	writer.Key("width");
	writer.Int((int)std::lroundf(_windowRect.Width));
	writer.Key("height");
	writer.Int((int)std::lroundf(_windowRect.Height));
	writer.Key("maximized");
	writer.Bool(_isWindowMaximized);
	writer.EndObject();

	writer.EndObject();

	if (!Win32Utils::WriteTextFile(StrUtils::ConcatW(_workingDir, CONFIG_PATH).c_str(), json.GetString())) {
		Logger::Get().Error("保存配置失败");
		return false;
	}

	return true;
}

void Settings::IsPortableMode(bool value) {
	if (_isPortableMode == value) {
		return;
	}

	if (!value) {
		// 关闭便携模式需删除本地配置文件
		// 不关心是否成功
		DeleteFile(StrUtils::ConcatW(_workingDir, CONFIG_PATH).c_str());
	}

	Logger::Get().Info(value ? "已开启便携模式" : "已关闭便携模式");

	_isPortableMode = value;
	_workingDir = GetWorkingDir(value);

	// 确保 WorkingDir 存在
	if (!Win32Utils::CreateDir(_workingDir.c_str(), true)) {
		Logger::Get().Error(StrUtils::Concat("创建文件夹", StrUtils::UTF16ToUTF8(_workingDir), "失败"));
	}
}

void Settings::Theme(int value) {
	assert(value >= 0 && value <= 2);

	if (_theme == value) {
		return;
	}

	static constexpr const char* SETTINGS[] = { "浅色","深色","系统" };
	Logger::Get().Info(StrUtils::Concat("主题已更改为：", SETTINGS[value]));

	_theme = value;
	_themeChangedEvent(*this, value);
}

winrt::event_token Settings::ThemeChanged(Windows::Foundation::EventHandler<int> const& handler) {
	return _themeChangedEvent.add(handler);
}

void Settings::ThemeChanged(winrt::event_token const& token) {
	_themeChangedEvent.remove(token);
}

Magpie::App::HotkeySettings Settings::GetHotkey(HotkeyAction action) const {
	return _hotkeys[(size_t)action];
}

void Settings::SetHotkey(HotkeyAction action, Magpie::App::HotkeySettings const& value) {
	if (_hotkeys[(size_t)action].Equals(value)) {
		return;
	}

	_hotkeys[(size_t)action] = value;
	_hotkeyChangedEvent(*this, action);
}

event_token Settings::HotkeyChanged(EventHandler<HotkeyAction> const& handler) {
	return _hotkeyChangedEvent.add(handler);
}

void Settings::HotkeyChanged(event_token const& token) {
	_hotkeyChangedEvent.remove(token);
}

}
