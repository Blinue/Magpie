#include "pch.h"
#include "AppSettings.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include "Logger.h"
#include "HotkeyHelper.h"
#include "ScalingProfile.h"
#include "CommonSharedConstants.h"
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>


namespace winrt::Magpie::App {

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

bool LoadBoolSettingItem(const rapidjson::GenericObject<false, rapidjson::Value>& obj, const char* nodeName, bool& result) {
	auto node = obj.FindMember(nodeName);
	if (node != obj.MemberEnd()) {
		if (!node->value.IsBool()) {
			return false;
		}

		result = node->value.GetBool();
	}

	return true;
}

bool LoadBoolSettingItem(
	const rapidjson::GenericObject<false, rapidjson::Value>& obj,
	const char* nodeName,
	std::optional<bool>& result
) {
	auto node = obj.FindMember(nodeName);
	if (node == obj.MemberEnd()) {
		result = std::nullopt;
		return true;
	}

	if (!node->value.IsBool()) {
		return false;
	}

	result = node->value.GetBool();
	return true;
}

bool LoadUIntSettingItem(
	const rapidjson::GenericObject<false, rapidjson::Value>& obj,
	const char* nodeName,
	std::optional<unsigned int>& result
) {
	auto node = obj.FindMember(nodeName);
	if (node == obj.MemberEnd()) {
		result = std::nullopt;
		return true;
	}

	if (!node->value.IsUint()) {
		return false;
	}

	result = node->value.GetUint();
	return true;
}

bool LoadDoubleSettingItem(
	const rapidjson::GenericObject<false, rapidjson::Value>& obj,
	const char* nodeName,
	std::optional<double>& result
) {
	auto node = obj.FindMember(nodeName);
	if (node == obj.MemberEnd()) {
		result = std::nullopt;
		return true;
	}

	if (!node->value.IsDouble()) {
		return false;
	}

	result = node->value.GetDouble();
	return true;
}

bool LoadStringSettingItem(
	const rapidjson::GenericObject<false, rapidjson::Value>& obj,
	const char* nodeName,
	std::optional<const char*>& result
) {
	auto node = obj.FindMember(nodeName);
	if (node == obj.MemberEnd()) {
		result = std::nullopt;
		return true;
	}

	if (!node->value.IsString()) {
		return false;
	}

	result = node->value.GetString();
	return true;
}

bool AppSettings::Initialize() {
	Logger& logger = Logger::Get();

	// 若程序所在目录存在配置文件则为便携模式
	_isPortableMode = Win32Utils::FileExists(CommonSharedConstants::CONFIG_PATH_W);
	_workingDir = GetWorkingDir(_isPortableMode);

	std::wstring configPath = StrUtils::ConcatW(_workingDir, CommonSharedConstants::CONFIG_PATH_W);

	if (!Win32Utils::FileExists(configPath.c_str())) {
		logger.Info("不存在配置文件");
		return true;
	}

	std::string configText;
	if (!Win32Utils::ReadTextFile(configPath.c_str(), configText)) {
		logger.Error("读取配置文件失败");
		return false;
	}

	if (!_LoadSettings(configText)) {
		logger.Error("解析配置文件失败");
		return false;
	}

	_SetDefaultHotkeys();
	return true;
}

void WriteScalingProfile(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const ScalingProfile& scalingProfile) {
	writer.StartObject();
	if (!scalingProfile.Name().empty()) {
		writer.Key("name");
		writer.String(StrUtils::UTF16ToUTF8(scalingProfile.Name()).c_str());
		writer.Key("pathRule");
		writer.String(StrUtils::UTF16ToUTF8(scalingProfile.PathRule()).c_str());
		writer.Key("classNameRule");
		writer.String(StrUtils::UTF16ToUTF8(scalingProfile.ClassNameRule()).c_str());
	}

	writer.Key("croppingEnabled");
	writer.Bool(scalingProfile.IsCroppingEnabled());

	Magpie::Runtime::MagSettings magSettings = scalingProfile.MagSettings();

	writer.Key("captureMode");
	writer.Uint((unsigned int)magSettings.CaptureMode());
	writer.Key("multiMonitorUsage");
	writer.Uint((unsigned int)magSettings.MultiMonitorUsage());
	writer.Key("graphicsAdapter");
	writer.Uint(magSettings.GraphicsAdapter());
	writer.Key("3dGameMode");
	writer.Bool(magSettings.Is3DGameMode());
	writer.Key("showFPS");
	writer.Bool(magSettings.IsShowFPS());
	writer.Key("VSync");
	writer.Bool(magSettings.IsVSync());
	writer.Key("tripleBuffering");
	writer.Bool(magSettings.IsTripleBuffering());
	writer.Key("disableWindowResizing");
	writer.Bool(magSettings.IsDisableWindowResizing());
	writer.Key("reserveTitleBar");
	writer.Bool(magSettings.IsReserveTitleBar());
	
	{
		Magpie::Runtime::Cropping cropping = magSettings.Cropping();

		writer.Key("cropping");
		writer.StartObject();
		writer.Key("left");
		writer.Double(cropping.Left);
		writer.Key("top");
		writer.Double(cropping.Top);
		writer.Key("right");
		writer.Double(cropping.Right);
		writer.Key("bottom");
		writer.Double(cropping.Bottom);
		writer.EndObject();
	}

	writer.Key("adjustCursorSpeed");
	writer.Bool(magSettings.IsAdjustCursorSpeed());
	writer.Key("drawCursor");
	writer.Bool(magSettings.IsDrawCursor());

	writer.EndObject();
}

bool LoadScalingProfile(const rapidjson::GenericObject<false, rapidjson::Value>& scalingConfigObj, ScalingProfile& scalingProfile) {
	std::optional<const char*> strValue;
	std::optional<unsigned int> uintValue;
	std::optional<bool> boolValue;
	std::optional<double> doubleValue;

	if (!LoadStringSettingItem(scalingConfigObj, "name", strValue)) {
		return false;
	}
	if (strValue.has_value()) {
		scalingProfile.Name(StrUtils::UTF8ToUTF16(strValue.value()));
	}
	
	if (!LoadStringSettingItem(scalingConfigObj, "pathRule", strValue)) {
		return false;
	}
	if (strValue.has_value()) {
		scalingProfile.PathRule(StrUtils::UTF8ToUTF16(strValue.value()));
	}

	if (!LoadStringSettingItem(scalingConfigObj, "classNameRule", strValue)) {
		return false;
	}
	if (strValue.has_value()) {
		scalingProfile.ClassNameRule(StrUtils::UTF8ToUTF16(strValue.value()));
	}

	if (!LoadBoolSettingItem(scalingConfigObj, "croppingEnabled", boolValue)) {
		return false;
	}
	if (boolValue.has_value()) {
		scalingProfile.IsCroppingEnabled(boolValue.value());
	}

	Magpie::Runtime::MagSettings magSettings = scalingProfile.MagSettings();

	if (!LoadUIntSettingItem(scalingConfigObj, "captureMode", uintValue)) {
		return false;
	}
	if (uintValue.has_value()) {
		magSettings.CaptureMode((Magpie::Runtime::CaptureMode)uintValue.value());
	}

	if (!LoadUIntSettingItem(scalingConfigObj, "multiMonitorUsage", uintValue)) {
		return false;
	}
	if (uintValue.has_value()) {
		magSettings.MultiMonitorUsage((Magpie::Runtime::MultiMonitorUsage)uintValue.value());
	}

	if (!LoadUIntSettingItem(scalingConfigObj, "graphicsAdapter", uintValue)) {
		return false;
	}
	if (uintValue.has_value()) {
		magSettings.GraphicsAdapter(uintValue.value());
	}

	if (!LoadBoolSettingItem(scalingConfigObj, "3dGameMode", boolValue)) {
		return false;
	}
	if (boolValue.has_value()) {
		magSettings.Is3DGameMode(boolValue.value());
	}

	if (!LoadBoolSettingItem(scalingConfigObj, "showFPS", boolValue)) {
		return false;
	}
	if (boolValue.has_value()) {
		magSettings.IsShowFPS(boolValue.value());
	}

	if (!LoadBoolSettingItem(scalingConfigObj, "VSync", boolValue)) {
		return false;
	}
	if (boolValue.has_value()) {
		magSettings.IsVSync(boolValue.value());
	}

	if (!LoadBoolSettingItem(scalingConfigObj, "tripleBuffering", boolValue)) {
		return false;
	}
	if (boolValue.has_value()) {
		magSettings.IsTripleBuffering(boolValue.value());
	}

	if (!LoadBoolSettingItem(scalingConfigObj, "disableWindowResizing", boolValue)) {
		return false;
	}
	if (boolValue.has_value()) {
		magSettings.IsDisableWindowResizing(boolValue.value());
	}

	if (!LoadBoolSettingItem(scalingConfigObj, "reserveTitleBar", boolValue)) {
		return false;
	}
	if (boolValue.has_value()) {
		magSettings.IsReserveTitleBar(boolValue.value());
	}

	{
		auto croppingNode = scalingConfigObj.FindMember("cropping");
		if (croppingNode != scalingConfigObj.MemberEnd()) {
			if (!croppingNode->value.IsObject()) {
				return false;
			}

			const auto& croppingObj = croppingNode->value.GetObj();

			Magpie::Runtime::Cropping cropping{};

			if (!LoadDoubleSettingItem(croppingObj, "left", doubleValue) || !doubleValue.has_value()) {
				return false;
			}
			cropping.Left = doubleValue.value();

			if (!LoadDoubleSettingItem(croppingObj, "top", doubleValue) || !doubleValue.has_value()) {
				return false;
			}
			cropping.Top = doubleValue.value();

			if (!LoadDoubleSettingItem(croppingObj, "right", doubleValue) || !doubleValue.has_value()) {
				return false;
			}
			cropping.Right = doubleValue.value();

			if (!LoadDoubleSettingItem(croppingObj, "bottom", doubleValue) || !doubleValue.has_value()) {
				return false;
			}
			cropping.Bottom = doubleValue.value();

			magSettings.Cropping(cropping);
		}
	}

	if (!LoadBoolSettingItem(scalingConfigObj, "adjustCursorSpeed", boolValue)) {
		return false;
	}
	if (boolValue.has_value()) {
		magSettings.IsAdjustCursorSpeed(boolValue.value());
	}

	if (!LoadBoolSettingItem(scalingConfigObj, "drawCursor", boolValue)) {
		return false;
	}
	if (boolValue.has_value()) {
		magSettings.IsDrawCursor(boolValue.value());
	}

	return true;
}

bool AppSettings::Save() {
	std::wstring configDir = StrUtils::ConcatW(_workingDir, L"config");
	if (!Win32Utils::CreateDir(configDir)) {
		Logger::Get().Error("创建配置文件夹失败");
		return false;
	}

	rapidjson::StringBuffer json;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(json);
	writer.StartObject();

	writer.Key("theme");
	writer.Uint(_theme);

	{
		HWND hwndHost = (HWND)Application::Current().as<App>().HwndHost();

		WINDOWPLACEMENT wp{};
		wp.length = sizeof(wp);
		if (GetWindowPlacement(hwndHost, &wp)) {
			writer.Key("windowPos");
			writer.StartObject();
			writer.Key("x");
			writer.Int((int)wp.rcNormalPosition.left);
			writer.Key("y");
			writer.Int((int)wp.rcNormalPosition.top);
			writer.Key("width");
			writer.Int(int(wp.rcNormalPosition.right - wp.rcNormalPosition.left));
			writer.Key("height");
			writer.Int(int(wp.rcNormalPosition.bottom - wp.rcNormalPosition.top));
			writer.Key("maximized");
			writer.Bool(wp.showCmd == SW_MAXIMIZE);
			writer.EndObject();
		} else {
			Logger::Get().Win32Error("GetWindowPlacement 失败");
		}
	}

	writer.Key("hotkeys");
	writer.StartObject();
	writer.Key("scale");
	writer.String(StrUtils::UTF16ToUTF8(_hotkeys[(size_t)HotkeyAction::Scale].ToString()).c_str());
	writer.Key("overlay");
	writer.String(StrUtils::UTF16ToUTF8(_hotkeys[(size_t)HotkeyAction::Overlay].ToString()).c_str());
	writer.EndObject();

	writer.Key("autoRestore");
	writer.Bool(_isAutoRestore);
	writer.Key("downCount");
	writer.Uint(_downCount);
	writer.Key("breakpointMode");
	writer.Bool(_isBreakpointMode);
	writer.Key("disableEffectCache");
	writer.Bool(_isDisableEffectCache);
	writer.Key("saveEffectSources");
	writer.Bool(_isSaveEffectSources);
	writer.Key("warningsAreErrors");
	writer.Bool(_isWarningsAreErrors);
	writer.Key("simulateExclusiveFullscreen");
	writer.Bool(_isSimulateExclusiveFullscreen);

	writer.Key("scalingProfiles");
	writer.StartArray();
	WriteScalingProfile(writer, _defaultScalingProfile);
	for (const ScalingProfile& rule : _scalingProfiles) {
		WriteScalingProfile(writer, rule);
	}
	writer.EndArray();

	writer.EndObject();

	if (!Win32Utils::WriteTextFile(StrUtils::ConcatW(_workingDir, CommonSharedConstants::CONFIG_PATH_W).c_str(), json.GetString())) {
		Logger::Get().Error("保存配置失败");
		return false;
	}

	return true;
}

void AppSettings::IsPortableMode(bool value) {
	if (_isPortableMode == value) {
		return;
	}

	if (!value) {
		// 关闭便携模式需删除本地配置文件
		// 不关心是否成功
		DeleteFile(StrUtils::ConcatW(_workingDir, CommonSharedConstants::CONFIG_PATH_W).c_str());
	}

	Logger::Get().Info(value ? "已开启便携模式" : "已关闭便携模式");

	_isPortableMode = value;
	_workingDir = GetWorkingDir(value);

	// 确保 WorkingDir 存在
	if (!Win32Utils::CreateDir(_workingDir.c_str(), true)) {
		Logger::Get().Error(StrUtils::Concat("创建文件夹", StrUtils::UTF16ToUTF8(_workingDir), "失败"));
	}
}

void AppSettings::Theme(uint32_t value) {
	assert(value <= 2);

	if (_theme == value) {
		return;
	}

	static constexpr const char* SETTINGS[] = { "浅色","深色","系统" };
	Logger::Get().Info(StrUtils::Concat("主题已更改为：", SETTINGS[value]));

	_theme = value;
	_themeChangedEvent(value);
}

void AppSettings::SetHotkey(HotkeyAction action, const Magpie::App::HotkeySettings& value) {
	if (_hotkeys[(size_t)action] == value) {
		return;
	}

	_hotkeys[(size_t)action] = value;
	Logger::Get().Info(fmt::format("热键 {} 已更改为 {}", HotkeyHelper::ToString(action), StrUtils::UTF16ToUTF8(value.ToString())));
	_hotkeyChangedEvent(action);
}

void AppSettings::IsAutoRestore(bool value) noexcept {
	if (_isAutoRestore == value) {
		return;
	}

	_isAutoRestore = value;
	_isAutoRestoreChangedEvent(value);
}

void AppSettings::DownCount(uint32_t value) noexcept {
	if (_downCount == value) {
		return;
	}

	_downCount = value;
	_downCountChangedEvent(value);
}

// 遇到不合法的配置项会失败，因此用户不应直接编辑配置文件
bool AppSettings::_LoadSettings(std::string text) {
	if (text.empty()) {
		Logger::Get().Info("配置文件为空");
		return true;
	}

	rapidjson::Document doc;
	if (doc.Parse(text.c_str(), text.size()).HasParseError()) {
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
			if (!themeNode->value.IsUint()) {
				return false;
			}

			_theme = themeNode->value.GetUint();
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
	{
		auto hotkeysNode = root.FindMember("hotkeys");
		if (hotkeysNode != root.MemberEnd()) {
			if (!hotkeysNode->value.IsObject()) {
				return false;
			}

			const auto& hotkeysObj = hotkeysNode->value.GetObj();

			auto scaleNode = hotkeysObj.FindMember("scale");
			if (scaleNode != hotkeysObj.MemberEnd() && scaleNode->value.IsString()) {
				_hotkeys[(size_t)HotkeyAction::Scale].FromString(StrUtils::UTF8ToUTF16(scaleNode->value.GetString()));
			}

			auto overlayNode = hotkeysObj.FindMember("overlay");
			if (overlayNode != hotkeysObj.MemberEnd() && overlayNode->value.IsString()) {
				_hotkeys[(size_t)HotkeyAction::Overlay].FromString(StrUtils::UTF8ToUTF16(overlayNode->value.GetString()));
			}
		}
	}

	if (!LoadBoolSettingItem(root, "autoRestore", _isAutoRestore)) {
		return false;
	}

	{
		auto downCountNode = root.FindMember("downCount");
		if (downCountNode != root.MemberEnd()) {
			if (!downCountNode->value.IsUint()) {
				return false;
			}

			_downCount = downCountNode->value.GetUint();
		}
	}

	if (!LoadBoolSettingItem(root, "breakpointMode", _isBreakpointMode)) {
		return false;
	}
	if (!LoadBoolSettingItem(root, "disableEffectCache", _isDisableEffectCache)) {
		return false;
	}
	if (!LoadBoolSettingItem(root, "saveEffectSources", _isSaveEffectSources)) {
		return false;
	}
	if (!LoadBoolSettingItem(root, "warningsAreErrors", _isWarningsAreErrors)) {
		return false;
	}
	if (!LoadBoolSettingItem(root, "simulateExclusiveFullscreen", _isSimulateExclusiveFullscreen)) {
		return false;
	}

	{
		auto scaleConfigsNode = root.FindMember("scalingProfiles");
		if (scaleConfigsNode != root.MemberEnd()) {
			if (!scaleConfigsNode->value.IsArray()) {
				return false;
			}

			const auto& scaleRulesArray = scaleConfigsNode->value.GetArray();

			const rapidjson::SizeType size = scaleRulesArray.Size();
			if (size > 0) {
				if (!scaleRulesArray[0].IsObject()) {
					return false;
				}

				if (!LoadScalingProfile(scaleRulesArray[0].GetObj(), _defaultScalingProfile)) {
					return false;
				}

				_defaultScalingProfile.Name(L"");
				_defaultScalingProfile.PathRule(L"");
				_defaultScalingProfile.ClassNameRule(L"");

				if (size > 1) {
					_scalingProfiles.resize(size - 1);
					for (rapidjson::SizeType i = 1; i < size; ++i) {
						if (!scaleRulesArray[i].IsObject()) {
							return false;
						}

						ScalingProfile& rule = _scalingProfiles[(size_t)i - 1];

						if (!LoadScalingProfile(scaleRulesArray[i].GetObj(), rule)) {
							return false;
						}

						if (rule.Name().empty() || rule.PathRule().empty() || rule.ClassNameRule().empty()) {
							return false;
						}
					}
				}
			}
		}
	}

	return true;
}

void AppSettings::_SetDefaultHotkeys() {
	HotkeySettings& scaleHotkey = _hotkeys[(size_t)HotkeyAction::Scale];
	if (scaleHotkey.IsEmpty()) {
		scaleHotkey.Win(true);
		scaleHotkey.Shift(true);
		scaleHotkey.Code('A');
	}

	HotkeySettings& overlayHotkey = _hotkeys[(size_t)HotkeyAction::Overlay];
	if (overlayHotkey.IsEmpty()) {
		overlayHotkey.Win(true);
		overlayHotkey.Shift(true);
		overlayHotkey.Code('D');
	}
}

}
