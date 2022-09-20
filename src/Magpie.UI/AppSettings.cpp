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
#include "AutoStartHelper.h"
#include <Magpie.Core.h>

using namespace Magpie::Core;


namespace winrt::Magpie::UI {

static bool LoadBoolSettingItem(
	const rapidjson::GenericObject<false, rapidjson::Value>& obj,
	const char* nodeName,
	bool& result,
	bool required = false
) {
	auto node = obj.FindMember(nodeName);
	if (node == obj.MemberEnd()) {
		return !required;
	}

	if (!node->value.IsBool()) {
		return false;
	}

	result = node->value.GetBool();
	return true;
}

static bool LoadBoolFlagSettingItem(
	const rapidjson::GenericObject<false, rapidjson::Value>& obj,
	const char* nodeName,
	uint32_t flagBit,
	uint32_t& flags
) {
	auto node = obj.FindMember(nodeName);
	if (node == obj.MemberEnd()) {
		return true;
	}

	if (!node->value.IsBool()) {
		return false;
	}

	if (node->value.GetBool()) {
		flags |= flagBit;
	} else {
		flags &= ~flagBit;
	}

	return true;
}

static bool LoadUIntSettingItem(
	const rapidjson::GenericObject<false, rapidjson::Value>& obj,
	const char* nodeName,
	uint32_t& result,
	bool required = false
) {
	auto node = obj.FindMember(nodeName);
	if (node == obj.MemberEnd()) {
		return !required;
	}

	if (!node->value.IsUint()) {
		return false;
	}

	result = node->value.GetUint();
	return true;
}

static bool LoadIntSettingItem(
	const rapidjson::GenericObject<false, rapidjson::Value>& obj,
	const char* nodeName,
	int& result,
	bool required = false
) {
	auto node = obj.FindMember(nodeName);
	if (node == obj.MemberEnd()) {
		return !required;
	}

	if (!node->value.IsInt()) {
		return false;
	}

	result = node->value.GetInt();
	return true;
}

static bool LoadFloatSettingItem(
	const rapidjson::GenericObject<false, rapidjson::Value>& obj,
	const char* nodeName,
	float& result,
	bool required = false
) {
	auto node = obj.FindMember(nodeName);
	if (node == obj.MemberEnd()) {
		return !required;
	}

	if (!node->value.IsNumber()) {
		return false;
	}

	result = node->value.GetFloat();
	return true;
}

static bool LoadStringSettingItem(
	const rapidjson::GenericObject<false, rapidjson::Value>& obj,
	const char* nodeName,
	std::wstring& result,
	bool required = false
) {
	auto node = obj.FindMember(nodeName);
	if (node == obj.MemberEnd()) {
		return !required;
	}

	if (!node->value.IsString()) {
		return false;
	}

	result = StrUtils::UTF8ToUTF16(node->value.GetString());
	return true;
}

static void WriteScaleMode(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const ScalingMode& scaleMode) {
	writer.StartObject();
	writer.Key("name");
	writer.String(StrUtils::UTF16ToUTF8(scaleMode.name).c_str());
	if (!scaleMode.effects.empty()) {
		writer.Key("effects");
		writer.StartArray();
		for (const auto& effect : scaleMode.effects) {
			writer.StartObject();
			writer.Key("name");
			writer.String(StrUtils::UTF16ToUTF8(effect.name).c_str());
			if (!effect.parameters.empty()) {
				writer.Key("parameters");
				writer.StartObject();
				for (const auto& [name, value] : effect.parameters) {
					writer.Key(StrUtils::UTF16ToUTF8(name).c_str());
					writer.Double(value);
				}
				writer.EndObject();
			}

			if (effect.HasScale()) {
				writer.Key("scaleType");
				writer.Uint((uint32_t)effect.scaleType);
				writer.Key("scale");
				writer.StartObject();
				writer.Key("x");
				writer.Double(effect.scale.first);
				writer.Key("y");
				writer.Double(effect.scale.second);
				writer.EndObject();
			}

			writer.EndObject();
		}
		writer.EndArray();
	}
	writer.EndObject();
}

static void WriteScalingProfile(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const ScalingProfile& scalingProfile) {
	writer.StartObject();
	if (!scalingProfile.name.empty()) {
		writer.Key("name");
		writer.String(StrUtils::UTF16ToUTF8(scalingProfile.name).c_str());
		writer.Key("packaged");
		writer.Bool(scalingProfile.isPackaged);
		writer.Key("pathRule");
		writer.String(StrUtils::UTF16ToUTF8(scalingProfile.pathRule).c_str());
		writer.Key("classNameRule");
		writer.String(StrUtils::UTF16ToUTF8(scalingProfile.classNameRule).c_str());
	}

	writer.Key("scalingMode");
	writer.Int(scalingProfile.scalingMode);
	writer.Key("captureMode");
	writer.Uint((uint32_t)scalingProfile.captureMode);
	writer.Key("multiMonitorUsage");
	writer.Uint((uint32_t)scalingProfile.multiMonitorUsage);
	writer.Key("graphicsAdapter");
	writer.Uint(scalingProfile.graphicsAdapter);

	writer.Key("disableWindowResizing");
	writer.Bool(scalingProfile.IsDisableWindowResizing());
	writer.Key("3DGameMode");
	writer.Bool(scalingProfile.Is3DGameMode());
	writer.Key("showFPS");
	writer.Bool(scalingProfile.IsShowFPS());
	writer.Key("VSync");
	writer.Bool(scalingProfile.IsVSync());
	writer.Key("tripleBuffering");
	writer.Bool(scalingProfile.IsTripleBuffering());
	writer.Key("reserveTitleBar");
	writer.Bool(scalingProfile.IsReserveTitleBar());
	writer.Key("adjustCursorSpeed");
	writer.Bool(scalingProfile.IsAdjustCursorSpeed());
	writer.Key("drawCursor");
	writer.Bool(scalingProfile.IsDrawCursor());
	writer.Key("disableDirectFlip");
	writer.Bool(scalingProfile.IsDisableDirectFlip());

	writer.Key("cursorScaling");
	writer.Uint((uint32_t)scalingProfile.cursorScaling);
	writer.Key("customCursorScaling");
	writer.Double(scalingProfile.customCursorScaling);
	writer.Key("cursorInterpolationMode");
	writer.Uint((uint32_t)scalingProfile.cursorInterpolationMode);

	writer.Key("croppingEnabled");
	writer.Bool(scalingProfile.isCroppingEnabled);
	writer.Key("cropping");
	writer.StartObject();
	writer.Key("left");
	writer.Double(scalingProfile.cropping.Left);
	writer.Key("top");
	writer.Double(scalingProfile.cropping.Top);
	writer.Key("right");
	writer.Double(scalingProfile.cropping.Right);
	writer.Key("bottom");
	writer.Double(scalingProfile.cropping.Bottom);
	writer.EndObject();

	writer.EndObject();
}

static bool LoadScalingProfile(const rapidjson::GenericObject<false, rapidjson::Value>& scalingConfigObj, ScalingProfile& scalingProfile, bool isDefault = false) {
	if (!isDefault) {
		if (!LoadStringSettingItem(scalingConfigObj, "name", scalingProfile.name, true)) {
			return false;
		}

		if (!LoadBoolSettingItem(scalingConfigObj, "packaged", scalingProfile.isPackaged, true)) {
			return false;
		}

		if (!LoadStringSettingItem(scalingConfigObj, "pathRule", scalingProfile.pathRule, true)) {
			return false;
		}

		if (!LoadStringSettingItem(scalingConfigObj, "classNameRule", scalingProfile.classNameRule, true)) {
			return false;
		}
	}

	if (!LoadIntSettingItem(scalingConfigObj, "scalingMode", scalingProfile.scalingMode)) {
		return false;
	}

	if (!LoadUIntSettingItem(scalingConfigObj, "captureMode", (uint32_t&)scalingProfile.captureMode)) {
		return false;
	}

	if (!LoadUIntSettingItem(scalingConfigObj, "multiMonitorUsage", (uint32_t&)scalingProfile.multiMonitorUsage)) {
		return false;
	}

	if (!LoadUIntSettingItem(scalingConfigObj, "graphicsAdapter", scalingProfile.graphicsAdapter)) {
		return false;
	}

	if (!LoadBoolFlagSettingItem(scalingConfigObj, "disableWindowResizing", MagFlags::DisableDirectFlip, scalingProfile.flags)) {
		return false;
	}
	if (!LoadBoolFlagSettingItem(scalingConfigObj, "3DGameMode", MagFlags::Is3DGameMode, scalingProfile.flags)) {
		return false;
	}
	if (!LoadBoolFlagSettingItem(scalingConfigObj, "showFPS", MagFlags::ShowFPS, scalingProfile.flags)) {
		return false;
	}
	if (!LoadBoolFlagSettingItem(scalingConfigObj, "VSync", MagFlags::VSync, scalingProfile.flags)) {
		return false;
	}
	if (!LoadBoolFlagSettingItem(scalingConfigObj, "tripleBuffering", MagFlags::TripleBuffering, scalingProfile.flags)) {
		return false;
	}
	if (!LoadBoolFlagSettingItem(scalingConfigObj, "reserveTitleBar", MagFlags::ReserveTitleBar, scalingProfile.flags)) {
		return false;
	}
	if (!LoadBoolFlagSettingItem(scalingConfigObj, "adjustCursorSpeed", MagFlags::AdjustCursorSpeed, scalingProfile.flags)) {
		return false;
	}
	if (!LoadBoolFlagSettingItem(scalingConfigObj, "drawCursor", MagFlags::DrawCursor, scalingProfile.flags)) {
		return false;
	}
	if (!LoadBoolFlagSettingItem(scalingConfigObj, "disableDirectFlip", MagFlags::DisableDirectFlip, scalingProfile.flags)) {
		return false;
	}

	if (!LoadUIntSettingItem(scalingConfigObj, "cursorScaling", (uint32_t&)scalingProfile.cursorScaling)) {
		return false;
	}
	if (!LoadFloatSettingItem(scalingConfigObj, "customCursorScaling", scalingProfile.customCursorScaling)) {
		return false;
	}
	if (!LoadUIntSettingItem(scalingConfigObj, "cursorInterpolationMode", (uint32_t&)scalingProfile.cursorInterpolationMode)) {
		return false;
	}

	{
		if (!LoadBoolSettingItem(scalingConfigObj, "croppingEnabled", scalingProfile.isCroppingEnabled)) {
			return false;
		}

		auto croppingNode = scalingConfigObj.FindMember("cropping");
		if (croppingNode != scalingConfigObj.MemberEnd()) {
			if (!croppingNode->value.IsObject()) {
				return false;
			}

			const auto& croppingObj = croppingNode->value.GetObj();

			if (!LoadFloatSettingItem(croppingObj, "left", scalingProfile.cropping.Left, true)
				|| !LoadFloatSettingItem(croppingObj, "top", scalingProfile.cropping.Top, true)
				|| !LoadFloatSettingItem(croppingObj, "right", scalingProfile.cropping.Right, true)
				|| !LoadFloatSettingItem(croppingObj, "bottom", scalingProfile.cropping.Bottom, true)
				) {
				return false;
			}
		}
	}

	return true;
}

static bool LoadScalingMode(const rapidjson::GenericObject<false, rapidjson::Value>& scalingModeObj, ScalingMode& scalingMode) {
	if (!LoadStringSettingItem(scalingModeObj, "name", scalingMode.name)) {
		return false;
	}

	auto effectsNode = scalingModeObj.FindMember("effects");
	if (effectsNode != scalingModeObj.MemberEnd()) {
		if (!effectsNode->value.IsArray()) {
			return false;
		}

		auto effectsArray = effectsNode->value.GetArray();
		rapidjson::SizeType size = effectsArray.Size();
		scalingMode.effects.resize(size);

		for (rapidjson::SizeType i = 0; i < size; ++i) {
			if (!effectsArray[i].IsObject()) {
				return false;
			}

			auto elemObj = effectsArray[i].GetObj();
			EffectOption& effect = scalingMode.effects[i];

			if (!LoadStringSettingItem(elemObj, "name", effect.name)) {
				return false;
			}

			{
				auto parametersNode = elemObj.FindMember("parameters");
				if (parametersNode != elemObj.MemberEnd()) {
					if (!parametersNode->value.IsObject()) {
						return false;
					}

					for (auto& param : parametersNode->value.GetObj()) {
						std::wstring name = StrUtils::UTF8ToUTF16(param.name.GetString());

						if (!param.value.IsNumber()) {
							return false;
						}
						effect.parameters[name] = param.value.GetFloat();
					}
				}
			}

			if (!LoadUIntSettingItem(elemObj, "scaleType", (uint32_t&)effect.scaleType)) {
				return false;
			}

			{
				auto scaleNode = elemObj.FindMember("scale");
				if (scaleNode != elemObj.MemberEnd()) {
					if (!scaleNode->value.IsObject()) {
						return false;
					}

					auto scaleObj = scaleNode->value.GetObj();

					if (!LoadFloatSettingItem(scaleObj, "x", effect.scale.first, true)
						|| !LoadFloatSettingItem(scaleObj, "y", effect.scale.second, true)
					) {
						return false;
					}
				}
			}
		}
	}

	return true;
}

bool AppSettings::Initialize() {
	Logger& logger = Logger::Get();

	// 若程序所在目录存在配置文件则为便携模式
	_isPortableMode = Win32Utils::FileExists(
		StrUtils::ConcatW(CommonSharedConstants::CONFIG_DIR, CommonSharedConstants::CONFIG_NAME).c_str());
	_UpdateConfigPath();

	if (!Win32Utils::FileExists(_configPath.c_str())) {
		logger.Info("不存在配置文件");
		// 只有不存在配置文件时才生成默认缩放模式
		_SetDefaultScalingModes();
		return true;
	}

	std::string configText;
	if (!Win32Utils::ReadTextFile(_configPath.c_str(), configText)) {
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

bool AppSettings::Save() {
	if (!Win32Utils::CreateDir(_configDir)) {
		Logger::Get().Error("创建配置文件夹失败");
		return false;
	}

	rapidjson::StringBuffer json;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(json);
	writer.StartObject();

	writer.Key("theme");
	writer.Uint(_theme);

	{
		if (HWND hwndMain = (HWND)Application::Current().as<App>().HwndMain()) {
			WINDOWPLACEMENT wp{};
			wp.length = sizeof(wp);
			if (GetWindowPlacement(hwndMain, &wp)) {
				_windowRect = {
					(float)wp.rcNormalPosition.left,
					(float)wp.rcNormalPosition.top,
					float(wp.rcNormalPosition.right - wp.rcNormalPosition.left),
					float(wp.rcNormalPosition.bottom - wp.rcNormalPosition.top)
				};
				_isWindowMaximized = wp.showCmd == SW_MAXIMIZE;
				
			} else {
				Logger::Get().Win32Error("GetWindowPlacement 失败");
			}
		}

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
	writer.Key("alwaysRunAsElevated");
	writer.Bool(_isAlwaysRunAsElevated);
	writer.Key("showTrayIcon");
	writer.Bool(_isShowTrayIcon);
	writer.Key("inlineParams");
	writer.Bool(_isInlineParams);

	writer.Key("scalingModes");
	writer.StartArray();
	for (const ScalingMode& scalingMode : _scalingModes) {
		WriteScaleMode(writer, scalingMode);
	}
	writer.EndArray();

	writer.Key("scalingProfiles");
	writer.StartArray();
	WriteScalingProfile(writer, _defaultScalingProfile);
	for (const ScalingProfile& rule : _scalingProfiles) {
		WriteScalingProfile(writer, rule);
	}
	writer.EndArray();

	writer.EndObject();

	if (!Win32Utils::WriteTextFile(_configPath.c_str(), json.GetString())) {
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
		DeleteFile(StrUtils::ConcatW(_configDir, CommonSharedConstants::CONFIG_NAME).c_str());
	}

	Logger::Get().Info(value ? "已开启便携模式" : "已关闭便携模式");

	_isPortableMode = value;
	_UpdateConfigPath();
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

void AppSettings::SetHotkey(HotkeyAction action, const Magpie::UI::HotkeySettings& value) {
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

void AppSettings::IsAlwaysRunAsElevated(bool value) noexcept {
	if (_isAlwaysRunAsElevated == value) {
		return;
	}

	_isAlwaysRunAsElevated = value;
	std::wstring arguments;
	if (AutoStartHelper::IsAutoStartEnabled(arguments)) {
		// 更新启动任务
		AutoStartHelper::EnableAutoStart(value, _isShowTrayIcon ? arguments.c_str() : nullptr);
	}
}

void AppSettings::IsShowTrayIcon(bool value) noexcept {
	if (_isShowTrayIcon == value) {
		return;
	}

	_isShowTrayIcon = value;
	_isShowTrayIconChangedEvent(value);
}

// 遇到不合法的配置项会失败，因此用户不应直接编辑配置文件
bool AppSettings::_LoadSettings(std::string text) {
	if (text.empty()) {
		Logger::Get().Info("配置文件为空");
		return true;
	}

	rapidjson::Document doc;
	if (doc.Parse(text.c_str(), text.size()).HasParseError()) {
		Logger::Get().Error(fmt::format("解析配置失败\n\t错误码：{}", (int)doc.GetParseError()));
		return false;
	}

	if (!doc.IsObject()) {
		return false;
	}

	const auto& root = doc.GetObj();

	if (!LoadUIntSettingItem(root, "theme", _theme)) {
		return false;
	}

	{
		auto windowPosNode = root.FindMember("windowPos");
		if (windowPosNode != root.MemberEnd()) {
			if (!windowPosNode->value.IsObject()) {
				return false;
			}

			const auto& windowRectObj = windowPosNode->value.GetObj();

			if (!LoadFloatSettingItem(windowRectObj, "x", _windowRect.X, true)) {
				return false;
			}
			if (!LoadFloatSettingItem(windowRectObj, "y", _windowRect.Y, true)) {
				return false;
			}
			if (!LoadFloatSettingItem(windowRectObj, "width", _windowRect.Width, true)) {
				return false;
			}
			if (!LoadFloatSettingItem(windowRectObj, "height", _windowRect.Height, true)) {
				return false;
			}
			if (!LoadBoolSettingItem(windowRectObj, "maximized", _isWindowMaximized, true)) {
				return false;
			}
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

	if (!LoadUIntSettingItem(root, "downCount", _downCount)) {
		return false;
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
	if (!LoadBoolSettingItem(root, "alwaysRunAsElevated", _isAlwaysRunAsElevated)) {
		return false;
	}
	if (!LoadBoolSettingItem(root, "showTrayIcon", _isShowTrayIcon)) {
		return false;
	}
	if (!LoadBoolSettingItem(root, "inlineParams", _isInlineParams)) {
		return false;
	}

	{
		auto scalingModesNode = root.FindMember("scalingModes");
		if (scalingModesNode != root.MemberEnd()) {
			if (!scalingModesNode->value.IsArray()) {
				return false;
			}

			const auto& scalingModesArray = scalingModesNode->value.GetArray();
			const rapidjson::SizeType size = scalingModesArray.Size();
			_scalingModes.resize(size);

			for (rapidjson::SizeType i = 0; i < size; ++i) {
				if (!scalingModesArray[i].IsObject()) {
					return false;
				}

				if (!LoadScalingMode(scalingModesArray[i].GetObj(), _scalingModes[i])) {
					return false;
				}
			}
		}
	}

	{
		auto scaleProfilesNode = root.FindMember("scalingProfiles");
		if (scaleProfilesNode != root.MemberEnd()) {
			if (!scaleProfilesNode->value.IsArray()) {
				return false;
			}

			const auto& scaleProfilesArray = scaleProfilesNode->value.GetArray();

			const rapidjson::SizeType size = scaleProfilesArray.Size();
			if (size > 0) {
				if (!scaleProfilesArray[0].IsObject()) {
					return false;
				}

				if (!LoadScalingProfile(scaleProfilesArray[0].GetObj(), _defaultScalingProfile, true)) {
					return false;
				}

				if (size > 1) {
					_scalingProfiles.resize((size_t)size - 1);
					for (rapidjson::SizeType i = 1; i < size; ++i) {
						if (!scaleProfilesArray[i].IsObject()) {
							return false;
						}

						ScalingProfile& rule = _scalingProfiles[(size_t)i - 1];

						if (!LoadScalingProfile(scaleProfilesArray[i].GetObj(), rule)) {
							return false;
						}

						if (rule.name.empty() || rule.pathRule.empty() || rule.classNameRule.empty()) {
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
		scaleHotkey.win = true;
		scaleHotkey.shift = true;
		scaleHotkey.code = 'A';
	}

	HotkeySettings& overlayHotkey = _hotkeys[(size_t)HotkeyAction::Overlay];
	if (overlayHotkey.IsEmpty()) {
		overlayHotkey.win = true;
		overlayHotkey.shift = true;
		overlayHotkey.code = 'D';
	}
}

void AppSettings::_SetDefaultScalingModes() {
	{
		auto& lanczos = _scalingModes.emplace_back();
		lanczos.name = L"Lanczos";
		auto& lanczosEffect = lanczos.effects.emplace_back();
		lanczosEffect.name = L"Lanczos";
		lanczosEffect.scaleType = ScaleType::Fit;
	}
	{
		auto& fsr = _scalingModes.emplace_back();
		fsr.name = L"FSR";
		auto& easu = fsr.effects.emplace_back();
		easu.name = L"FSR_EASU";
		easu.scaleType = ScaleType::Fit;
		auto& rcas = fsr.effects.emplace_back();
		rcas.name = L"FSR_RCAS";
		rcas.parameters[L"sharpness"] = 0.87f;
	}
}

void AppSettings::_UpdateConfigPath() noexcept {
	if (_isPortableMode) {
		wchar_t curDir[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, curDir);

		_configDir = curDir;
		if (_configDir.back() != L'\\') {
			_configDir.push_back(L'\\');
		}
		_configDir += CommonSharedConstants::CONFIG_DIR;
	} else {
		wchar_t localAppDataDir[MAX_PATH];
		HRESULT hr = SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataDir);
		if (SUCCEEDED(hr)) {
			_configDir = StrUtils::ConcatW(
				localAppDataDir,
				localAppDataDir[StrUtils::StrLen(localAppDataDir) - 1] == L'\\' ? L"Magpie\\" : L"\\Magpie\\",
				CommonSharedConstants::CONFIG_DIR
			);
		} else {
			Logger::Get().ComError("SHGetFolderPath 失败", hr);
			_configDir = CommonSharedConstants::CONFIG_DIR;
		}
	}

	_configPath = _configDir + CommonSharedConstants::CONFIG_NAME;

	// 确保 ConfigDir 存在
	Win32Utils::CreateDir(_configDir.c_str(), true);
}

}
