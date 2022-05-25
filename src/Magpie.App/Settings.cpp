#include "pch.h"
#include "Settings.h"
#if __has_include("Settings.g.cpp")
#include "Settings.g.cpp"
#endif

#include "Utils.h"
#include "StrUtils.h"
#include "Logger.h"
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

using namespace winrt;
using namespace Windows::Foundation;


namespace winrt::Magpie::App::implementation {

bool Settings::Initialize(const hstring& workingDir) {
	_workingDir = workingDir;

	// 若程序所在目录存在配置文件则为便携模式
	std::wstring configPath = StrUtils::ConcatW(workingDir, L"config\\config.json");

	if (!Utils::FileExists(configPath.c_str())) {
		Logger::Get().Info("不存在配置文件");
		return true;
	}

	std::string configText;
	if (!Utils::ReadTextFile(configPath.c_str(), configText)) {
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
	auto theme = root.FindMember("theme");
	if (theme != root.MemberEnd()) {
		if (!theme->value.IsNumber()) {
			return false;
		}

		_theme = theme->value.GetInt();
	}

	return true;
}

bool Settings::Save() {
	std::wstring configDir = StrUtils::ConcatW(_workingDir, L"config");
	if (!Utils::DirExists(configDir.c_str()) && !CreateDirectory(configDir.c_str(), nullptr)) {
		Logger::Get().Error("创建配置文件夹失败");
		return false;
	}

	rapidjson::StringBuffer json;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(json);
	writer.StartObject();
	writer.Key("theme");
	writer.Int(_theme);
	writer.EndObject();

	if (!Utils::WriteTextFile(StrUtils::ConcatW(_workingDir, L"config\\config.json").c_str(), json.GetString())) {
		Logger::Get().Error("保存配置失败");
		return false;
	}

	return true;
}

bool Settings::IsPortableMode() {
	// 在当前目录中寻找配置文件
	return Utils::FileExists(L"config\\config.json");
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

void Settings::ThemeChanged(winrt::event_token const& token) noexcept {
	_themeChangedEvent.remove(token);
}

}
