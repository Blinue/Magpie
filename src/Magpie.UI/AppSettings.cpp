#include "pch.h"
#include "AppSettings.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include "Logger.h"
#include "HotkeyHelper.h"
#include "ScalingProfile.h"
#include "CommonSharedConstants.h"
#include <rapidjson/prettywriter.h>
#include "AutoStartHelper.h"
#include <Magpie.Core.h>
#include "ScalingModesService.h"
#include "JsonHelper.h"
#include "ScalingMode.h"

using namespace Magpie::Core;

namespace winrt::Magpie::UI {

static constexpr uint32_t SETTINGS_VERSION = 0;

_AppSettingsData::_AppSettingsData() {}

_AppSettingsData::~_AppSettingsData() {}

// 将热键存储为 uint32_t
// 不能存储为字符串，因为某些键有相同的名称，如句号和小键盘的点
static uint32_t EncodeHotkey(const HotkeySettings& hotkey) noexcept {
	uint32_t value = 0;
	value |= hotkey.code;
	if (hotkey.win) {
		value |= 0x100;
	}
	if (hotkey.ctrl) {
		value |= 0x200;
	}
	if (hotkey.alt) {
		value |= 0x400;
	}
	if (hotkey.shift) {
		value |= 0x800;
	}
	return value;
}

static void DecodeHotkey(uint32_t value, HotkeySettings& hotkey) noexcept {
	if (value > 0xfff) {
		return;
	}

	hotkey.code = value & 0xff;
	hotkey.win = value & 0x100;
	hotkey.ctrl = value & 0x200;
	hotkey.alt = value & 0x400;
	hotkey.shift = value & 0x800;
}

static void WriteScalingProfile(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const ScalingProfile& scalingProfile) noexcept {
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
	writer.Key("captureMethod");
	writer.Uint((uint32_t)scalingProfile.captureMethod);
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

static void ReplaceIcon(HINSTANCE hInst, HWND hWnd, bool large) noexcept {
	HICON hIconApp = NULL;
	LoadIconMetric(hInst, MAKEINTRESOURCE(CommonSharedConstants::IDI_APP), large ? LIM_LARGE : LIM_SMALL, &hIconApp);
	HICON hIconOld = (HICON)SendMessage(hWnd, WM_SETICON, large ? ICON_BIG : ICON_SMALL, (LPARAM)hIconApp);
	if (hIconOld) {
		DestroyIcon(hIconOld);
	}
}

static HRESULT CALLBACK TaskDialogCallback(
	HWND hWnd,
	UINT msg,
	WPARAM /*wParam*/,
	LPARAM /*lParam*/,
	LONG_PTR /*lpRefData*/
) {
	if (msg == TDN_CREATED) {
		// 将任务栏图标替换为 Magpie 的图标
		HINSTANCE hInst = GetModuleHandle(nullptr);
		ReplaceIcon(hInst, hWnd, true);
		ReplaceIcon(hInst, hWnd, false);

		// 删除标题栏中的图标
		INT_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
		SetWindowLongPtr(hWnd, GWL_STYLE, style & ~WS_SYSMENU);
	}

	return S_OK;
}

static void ShowErrorMessage(const wchar_t* mainInstruction, const wchar_t* content) {
	TASKDIALOGCONFIG tdc{ sizeof(TASKDIALOGCONFIG) };
	tdc.dwFlags = TDF_SIZE_TO_CONTENT;
	tdc.pszWindowTitle = L"错误";
	tdc.pszMainIcon = TD_ERROR_ICON;
	tdc.pszMainInstruction = mainInstruction;
	tdc.pszContent = content;
	tdc.pfCallback = TaskDialogCallback;
	tdc.cButtons = 1;
	TASKDIALOG_BUTTON button{ IDCANCEL, L"退出" };
	tdc.pButtons = &button;

	TaskDialogIndirect(&tdc, nullptr, nullptr, nullptr);
}

static bool ShowOkCancelWarningMessage(
	const wchar_t* mainInstruction,
	const wchar_t* content,
	const wchar_t* okText,
	const wchar_t* cancelText
) noexcept {
	TASKDIALOGCONFIG tdc{ sizeof(TASKDIALOGCONFIG) };
	tdc.dwFlags = TDF_SIZE_TO_CONTENT;
	tdc.pszWindowTitle = L"警告";
	tdc.pszMainIcon = TD_WARNING_ICON;
	tdc.pszMainInstruction = mainInstruction;
	tdc.pszContent = content;
	tdc.pfCallback = TaskDialogCallback;
	TASKDIALOG_BUTTON buttons[]{
		{IDOK, okText},
		{IDCANCEL, cancelText}
	};
	tdc.cButtons = (UINT)std::size(buttons);
	tdc.pButtons = buttons;

	int button = 0;
	TaskDialogIndirect(&tdc, &button, nullptr, nullptr);
	return button == IDOK;
}

AppSettings::~AppSettings() {}

bool AppSettings::Initialize() {
	Logger& logger = Logger::Get();

	// 若程序所在目录存在配置文件则为便携模式
	_isPortableMode = Win32Utils::FileExists(
		StrUtils::ConcatW(CommonSharedConstants::CONFIG_DIR, CommonSharedConstants::CONFIG_NAME).c_str());
	_UpdateConfigPath();

	logger.Info(StrUtils::Concat("便携模式：", _isPortableMode ? "是" : "否"));

	if (!Win32Utils::FileExists(_configPath.c_str())) {
		logger.Info("不存在配置文件");
		_SetDefaultScalingModes();
		_SetDefaultHotkeys();
		SaveAsync();
		return true;
	}

	std::string configText;
	if (!Win32Utils::ReadTextFile(_configPath.c_str(), configText)) {
		logger.Error("读取配置文件失败");
		ShowErrorMessage(L"读取配置文件失败", (L"配置文件路径：\n" + _configPath).c_str());
		return false;
	}

	if (configText.empty()) {
		Logger::Get().Info("配置文件为空");
		_SetDefaultScalingModes();
		_SetDefaultHotkeys();
		SaveAsync();
		return true;
	}

	rapidjson::Document doc;
	doc.ParseInsitu(configText.data());
	if (doc.HasParseError()) {
		Logger::Get().Error(fmt::format("解析配置失败\n\t错误码：{}", (int)doc.GetParseError()));
		ShowErrorMessage(L"配置文件不是合法的 JSON", (L"配置文件路径：\n" + _configPath).c_str());
		return false;
	}

	if (!doc.IsObject()) {
		Logger::Get().Error("配置文件根元素不是 Object");
		ShowErrorMessage(L"解析配置文件失败", (L"配置文件路径：\n" + _configPath).c_str());
		return false;
	}

	auto root = ((const rapidjson::Document&)doc).GetObj();

	uint32_t settingsVersion = 0;
	// 不存在 version 字段则视为 0
	JsonHelper::ReadUInt(root, "version", settingsVersion);

	if (settingsVersion > SETTINGS_VERSION) {
		Logger::Get().Warn("未知的配置文件版本");
		if (_isPortableMode) {
			if (!ShowOkCancelWarningMessage(nullptr, L"配置文件来自未知版本，可能无法正确解析。",
				L"继续使用", L"退出")
			) {
				return false;
			}
		} else {
			if (!ShowOkCancelWarningMessage(nullptr, L"全局配置文件来自未知版本，可能无法正确解析。",
				L"继续使用", L"启用便携模式")
			) {
				IsPortableMode(true);
				_SetDefaultScalingModes();
				_SetDefaultHotkeys();
				SaveAsync();
				return true;
			}
		}
	}

	_LoadSettings(root, settingsVersion);

	if (_SetDefaultHotkeys()) {
		SaveAsync();
	}
	return true;
}

bool AppSettings::Save() {
	_UpdateWindowPlacement();
	return _Save(*this);
}

fire_and_forget AppSettings::SaveAsync() {
	_UpdateWindowPlacement();

	// 拷贝当前配置
	_AppSettingsData data = *this;
	co_await resume_background();

	_Save(data);
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

	SaveAsync();
}

void AppSettings::Theme(uint32_t value) {
	assert(value <= 2);

	if (_theme == value) {
		return;
	}

	_theme = value;
	_themeChangedEvent(value);

	SaveAsync();
}

void AppSettings::SetHotkey(HotkeyAction action, const Magpie::UI::HotkeySettings& value) {
	if (_hotkeys[(size_t)action] == value) {
		return;
	}

	_hotkeys[(size_t)action] = value;
	Logger::Get().Info(fmt::format("热键 {} 已更改为 {}", HotkeyHelper::ToString(action), StrUtils::UTF16ToUTF8(value.ToString())));
	_hotkeyChangedEvent(action);

	SaveAsync();
}

void AppSettings::IsAutoRestore(bool value) noexcept {
	if (_isAutoRestore == value) {
		return;
	}

	_isAutoRestore = value;
	_isAutoRestoreChangedEvent(value);

	SaveAsync();
}

void AppSettings::DownCount(uint32_t value) noexcept {
	if (_downCount == value) {
		return;
	}

	_downCount = value;
	_downCountChangedEvent(value);

	SaveAsync();
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

	SaveAsync();
}

void AppSettings::IsShowTrayIcon(bool value) noexcept {
	if (_isShowTrayIcon == value) {
		return;
	}

	_isShowTrayIcon = value;
	_isShowTrayIconChangedEvent(value);

	SaveAsync();
}

void AppSettings::_UpdateWindowPlacement() noexcept {
	HWND hwndMain = (HWND)Application::Current().as<App>().HwndMain();
	if (!hwndMain) {
		return;
	}

	WINDOWPLACEMENT wp{ sizeof(wp) };
	if (!GetWindowPlacement(hwndMain, &wp)) {
		Logger::Get().Win32Error("GetWindowPlacement 失败");
		return;
	}

	_windowRect = {
		wp.rcNormalPosition.left,
		wp.rcNormalPosition.top,
		wp.rcNormalPosition.right - wp.rcNormalPosition.left,
		wp.rcNormalPosition.bottom - wp.rcNormalPosition.top
	};
	_isWindowMaximized = wp.showCmd == SW_MAXIMIZE;
}

bool AppSettings::_Save(const _AppSettingsData& data) noexcept {
	if (!Win32Utils::CreateDir(data._configDir)) {
		Logger::Get().Error("创建配置文件夹失败");
		return false;
	}

	rapidjson::StringBuffer json;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(json);
	writer.StartObject();

	writer.Key("version");
	writer.Uint(SETTINGS_VERSION);

	writer.Key("theme");
	writer.Uint(data._theme);

	writer.Key("windowPos");
	writer.StartObject();
	writer.Key("x");
	writer.Int(data._windowRect.left);
	writer.Key("y");
	writer.Int(data._windowRect.top);
	writer.Key("width");
	writer.Uint((uint32_t)data._windowRect.right);
	writer.Key("height");
	writer.Uint((uint32_t)data._windowRect.bottom);
	writer.Key("maximized");
	writer.Bool(data._isWindowMaximized);
	writer.EndObject();

	writer.Key("hotkeys");
	writer.StartObject();
	writer.Key("scale");
	writer.Uint(EncodeHotkey(data._hotkeys[(size_t)HotkeyAction::Scale]));
	writer.Key("overlay");
	writer.Uint(EncodeHotkey(data._hotkeys[(size_t)HotkeyAction::Overlay]));
	writer.EndObject();

	writer.Key("autoRestore");
	writer.Bool(data._isAutoRestore);
	writer.Key("downCount");
	writer.Uint(data._downCount);
	writer.Key("debugMode");
	writer.Bool(data._isDebugMode);
	writer.Key("disableEffectCache");
	writer.Bool(data._isDisableEffectCache);
	writer.Key("saveEffectSources");
	writer.Bool(data._isSaveEffectSources);
	writer.Key("warningsAreErrors");
	writer.Bool(data._isWarningsAreErrors);
	writer.Key("simulateExclusiveFullscreen");
	writer.Bool(data._isSimulateExclusiveFullscreen);
	writer.Key("alwaysRunAsElevated");
	writer.Bool(data._isAlwaysRunAsElevated);
	writer.Key("showTrayIcon");
	writer.Bool(data._isShowTrayIcon);
	writer.Key("inlineParams");
	writer.Bool(data._isInlineParams);
	writer.Key("autoCheckForUpdates");
	writer.Bool(data._isAutoCheckForUpdates);
	writer.Key("checkForPreviewUpdates");
	writer.Bool(data._isCheckForPreviewUpdates);

	if (!data._downscalingEffect.name.empty()) {
		writer.Key("downscalingEffect");
		writer.StartObject();
		writer.Key("name");
		writer.String(StrUtils::UTF16ToUTF8(data._downscalingEffect.name).c_str());
		if (!data._downscalingEffect.parameters.empty()) {
			writer.Key("parameters");
			writer.StartObject();
			for (const auto& [name, value] : data._downscalingEffect.parameters) {
				writer.Key(StrUtils::UTF16ToUTF8(name).c_str());
				writer.Double(value);
			}
			writer.EndObject();
		}
		writer.EndObject();
	}

	ScalingModesService::Get().Export(writer);

	writer.Key("scalingProfiles");
	writer.StartArray();
	WriteScalingProfile(writer, data._defaultScalingProfile);
	for (const ScalingProfile& rule : data._scalingProfiles) {
		WriteScalingProfile(writer, rule);
	}
	writer.EndArray();

	writer.EndObject();

	// 防止并行写入
	std::scoped_lock lk(_saveMutex);
	if (!Win32Utils::WriteTextFile(data._configPath.c_str(), { json.GetString(), json.GetLength() })) {
		Logger::Get().Error("保存配置失败");
		return false;
	}

	return true;
}

// 永远不会失败，遇到不合法的配置项则静默忽略
void AppSettings::_LoadSettings(const rapidjson::GenericObject<true, rapidjson::Value>& root, uint32_t /*version*/) {
	JsonHelper::ReadUInt(root, "theme", _theme);

	auto windowPosNode = root.FindMember("windowPos");
	if (windowPosNode != root.MemberEnd() && windowPosNode->value.IsObject()) {
		const auto& windowRectObj = windowPosNode->value.GetObj();

		int x = 0;
		int y = 0;
		if (JsonHelper::ReadInt(windowRectObj, "x", x, true)
			&& JsonHelper::ReadInt(windowRectObj, "y", y, true)) {
			_windowRect.left = x;
			_windowRect.top = y;
		}

		uint32_t width = 0;
		uint32_t height = 0;
		if (JsonHelper::ReadUInt(windowRectObj, "width", width, true)
			&& JsonHelper::ReadUInt(windowRectObj, "height", height, true)) {
			_windowRect.right = (LONG)width;
			_windowRect.bottom = (LONG)height;
		}

		JsonHelper::ReadBool(windowRectObj, "maximized", _isWindowMaximized);
	}

	auto hotkeysNode = root.FindMember("hotkeys");
	if (hotkeysNode != root.MemberEnd() && hotkeysNode->value.IsObject()) {
		const auto& hotkeysObj = hotkeysNode->value.GetObj();

		auto scaleNode = hotkeysObj.FindMember("scale");
		if (scaleNode != hotkeysObj.MemberEnd() && scaleNode->value.IsUint()) {
			DecodeHotkey(scaleNode->value.GetUint(), _hotkeys[(size_t)HotkeyAction::Scale]);
		}

		auto overlayNode = hotkeysObj.FindMember("overlay");
		if (overlayNode != hotkeysObj.MemberEnd() && overlayNode->value.IsUint()) {
			DecodeHotkey(overlayNode->value.GetUint(), _hotkeys[(size_t)HotkeyAction::Overlay]);
		}
	}

	JsonHelper::ReadBool(root, "autoRestore", _isAutoRestore);
	JsonHelper::ReadUInt(root, "downCount", _downCount);
	if (_downCount == 0 || _downCount > 5) {
		_downCount = 3;
	}
	JsonHelper::ReadBool(root, "debugMode", _isDebugMode);
	JsonHelper::ReadBool(root, "disableEffectCache", _isDisableEffectCache);
	JsonHelper::ReadBool(root, "saveEffectSources", _isSaveEffectSources);
	JsonHelper::ReadBool(root, "warningsAreErrors", _isWarningsAreErrors);
	JsonHelper::ReadBool(root, "simulateExclusiveFullscreen", _isSimulateExclusiveFullscreen);
	JsonHelper::ReadBool(root, "alwaysRunAsElevated", _isAlwaysRunAsElevated);
	JsonHelper::ReadBool(root, "showTrayIcon", _isShowTrayIcon);
	JsonHelper::ReadBool(root, "inlineParams", _isInlineParams);
	JsonHelper::ReadBool(root, "autoCheckForUpdates", _isAutoCheckForUpdates);
	JsonHelper::ReadBool(root, "checkForPreviewUpdates", _isCheckForPreviewUpdates);

	auto downscalingEffectNode = root.FindMember("downscalingEffect");
	if (downscalingEffectNode != root.MemberEnd() && downscalingEffectNode->value.IsObject()) {
		auto downscalingEffectObj = downscalingEffectNode->value.GetObj();

		JsonHelper::ReadString(downscalingEffectObj, "name", _downscalingEffect.name);
		if (!_downscalingEffect.name.empty()) {
			auto parametersNode = downscalingEffectObj.FindMember("parameters");
			if (parametersNode != downscalingEffectObj.MemberEnd() && parametersNode->value.IsObject()) {
				auto paramsObj = parametersNode->value.GetObj();
				_downscalingEffect.parameters.reserve(paramsObj.MemberCount());
				for (const auto& param : paramsObj) {
					if (!param.value.IsNumber()) {
						continue;
					}

					std::wstring name = StrUtils::UTF8ToUTF16(param.name.GetString());
					_downscalingEffect.parameters[name] = param.value.GetFloat();
				}
			}
		}
	}

	[[maybe_unused]] bool result = ScalingModesService::Get().Import(root, true);
	assert(result);

	auto scaleProfilesNode = root.FindMember("scalingProfiles");
	if (scaleProfilesNode != root.MemberEnd() && scaleProfilesNode->value.IsArray()) {
		const auto& scaleProfilesArray = scaleProfilesNode->value.GetArray();

		const rapidjson::SizeType size = scaleProfilesArray.Size();
		if (size > 0) {
			if (scaleProfilesArray[0].IsObject()) {
				// 解析默认缩放配置不会失败
				_LoadScalingProfile(scaleProfilesArray[0].GetObj(), _defaultScalingProfile, true);
			}

			if (size > 1) {
				_scalingProfiles.reserve((size_t)size - 1);
				for (rapidjson::SizeType i = 1; i < size; ++i) {
					if (!scaleProfilesArray[i].IsObject()) {
						continue;
					}

					ScalingProfile& rule = _scalingProfiles.emplace_back();
					if (!_LoadScalingProfile(scaleProfilesArray[i].GetObj(), rule)) {
						_scalingProfiles.pop_back();
						continue;
					}
				}
			}
		}
	}
}

bool AppSettings::_LoadScalingProfile(
	const rapidjson::GenericObject<true, rapidjson::Value>& scalingProfileObj,
	ScalingProfile& scalingProfile,
	bool isDefault
) {
	if (!isDefault) {
		if (!JsonHelper::ReadString(scalingProfileObj, "name", scalingProfile.name, true)) {
			return false;
		}

		{
			std::wstring_view nameView(scalingProfile.name);
			StrUtils::Trim(nameView);
			if (nameView.empty()) {
				return false;
			}
		}

		if (!JsonHelper::ReadBool(scalingProfileObj, "packaged", scalingProfile.isPackaged, true)) {
			return false;
		}

		if (!JsonHelper::ReadString(scalingProfileObj, "pathRule", scalingProfile.pathRule, true)
			|| scalingProfile.pathRule.empty()) {
			return false;
		}

		if (!JsonHelper::ReadString(scalingProfileObj, "classNameRule", scalingProfile.classNameRule, true)
			|| scalingProfile.classNameRule.empty()) {
			return false;
		}
	}

	JsonHelper::ReadInt(scalingProfileObj, "scalingMode", scalingProfile.scalingMode);
	if (scalingProfile.scalingMode < -1 || scalingProfile.scalingMode >= _scalingModes.size()) {
		scalingProfile.scalingMode = -1;
	}

	{
		uint32_t captureMethod = (uint32_t)CaptureMethod::GraphicsCapture;
		if (!JsonHelper::ReadUInt(scalingProfileObj, "captureMethod", captureMethod, true)) {
			// v0.10.0-preview1 使用 captureMode
			JsonHelper::ReadUInt(scalingProfileObj, "captureMode", captureMethod);
		}
		
		if (captureMethod > 3) {
			captureMethod = (uint32_t)CaptureMethod::GraphicsCapture;
		} else if (captureMethod == (uint32_t)CaptureMethod::DesktopDuplication) {
			// Desktop Duplication 捕获模式要求 Win10 20H1+
			if (!Win32Utils::GetOSVersion().Is20H1OrNewer()) {
				captureMethod = (uint32_t)CaptureMethod::GraphicsCapture;
			}
		}
		scalingProfile.captureMethod = (CaptureMethod)captureMethod;
	}

	{
		uint32_t multiMonitorUsage = (uint32_t)MultiMonitorUsage::Nearest;
		JsonHelper::ReadUInt(scalingProfileObj, "multiMonitorUsage", multiMonitorUsage);
		if (multiMonitorUsage > 2) {
			multiMonitorUsage = (uint32_t)MultiMonitorUsage::Nearest;
		}
		scalingProfile.multiMonitorUsage = (MultiMonitorUsage)multiMonitorUsage;
	}
	
	JsonHelper::ReadUInt(scalingProfileObj, "graphicsAdapter", scalingProfile.graphicsAdapter);
	JsonHelper::ReadBoolFlag(scalingProfileObj, "disableWindowResizing", MagFlags::DisableWindowResizing, scalingProfile.flags);
	JsonHelper::ReadBoolFlag(scalingProfileObj, "3DGameMode", MagFlags::Is3DGameMode, scalingProfile.flags);
	JsonHelper::ReadBoolFlag(scalingProfileObj, "showFPS", MagFlags::ShowFPS, scalingProfile.flags);
	JsonHelper::ReadBoolFlag(scalingProfileObj, "VSync", MagFlags::VSync, scalingProfile.flags);
	JsonHelper::ReadBoolFlag(scalingProfileObj, "tripleBuffering", MagFlags::TripleBuffering, scalingProfile.flags);
	JsonHelper::ReadBoolFlag(scalingProfileObj, "reserveTitleBar", MagFlags::ReserveTitleBar, scalingProfile.flags);
	JsonHelper::ReadBoolFlag(scalingProfileObj, "adjustCursorSpeed", MagFlags::AdjustCursorSpeed, scalingProfile.flags);
	JsonHelper::ReadBoolFlag(scalingProfileObj, "drawCursor", MagFlags::DrawCursor, scalingProfile.flags);
	JsonHelper::ReadBoolFlag(scalingProfileObj, "disableDirectFlip", MagFlags::DisableDirectFlip, scalingProfile.flags);

	{
		uint32_t cursorScaling = (uint32_t)CursorScaling::NoScaling;
		JsonHelper::ReadUInt(scalingProfileObj, "cursorScaling", cursorScaling);
		if (cursorScaling > 7) {
			cursorScaling = (uint32_t)CursorScaling::NoScaling;
		}
		scalingProfile.cursorScaling = (CursorScaling)cursorScaling;
	}
	
	JsonHelper::ReadFloat(scalingProfileObj, "customCursorScaling", scalingProfile.customCursorScaling);
	if (scalingProfile.customCursorScaling < 0) {
		scalingProfile.customCursorScaling = 1.0f;
	}

	{
		uint32_t cursorInterpolationMode = (uint32_t)CursorInterpolationMode::Nearest;
		JsonHelper::ReadUInt(scalingProfileObj, "cursorInterpolationMode", (uint32_t&)scalingProfile.cursorInterpolationMode);
		if (cursorInterpolationMode > 1) {
			cursorInterpolationMode = (uint32_t)CursorInterpolationMode::Nearest;
		}
		scalingProfile.cursorInterpolationMode = (CursorInterpolationMode)cursorInterpolationMode;
	}

	JsonHelper::ReadBool(scalingProfileObj, "croppingEnabled", scalingProfile.isCroppingEnabled);

	auto croppingNode = scalingProfileObj.FindMember("cropping");
	if (croppingNode != scalingProfileObj.MemberEnd() && croppingNode->value.IsObject()) {
		const auto& croppingObj = croppingNode->value.GetObj();

		if (!JsonHelper::ReadFloat(croppingObj, "left", scalingProfile.cropping.Left, true)
			|| scalingProfile.cropping.Left < 0
			|| !JsonHelper::ReadFloat(croppingObj, "top", scalingProfile.cropping.Top, true)
			|| scalingProfile.cropping.Top < 0
			|| !JsonHelper::ReadFloat(croppingObj, "right", scalingProfile.cropping.Right, true)
			|| scalingProfile.cropping.Right < 0
			|| !JsonHelper::ReadFloat(croppingObj, "bottom", scalingProfile.cropping.Bottom, true)
			|| scalingProfile.cropping.Bottom < 0
		) {
			scalingProfile.cropping = {};
		}
	}

	return true;
}

bool AppSettings::_SetDefaultHotkeys() {
	bool changed = false;

	HotkeySettings& scaleHotkey = _hotkeys[(size_t)HotkeyAction::Scale];
	if (scaleHotkey.IsEmpty()) {
		scaleHotkey.win = true;
		scaleHotkey.shift = true;
		scaleHotkey.code = 'A';

		changed = true;
	}

	HotkeySettings& overlayHotkey = _hotkeys[(size_t)HotkeyAction::Overlay];
	if (overlayHotkey.IsEmpty()) {
		overlayHotkey.win = true;
		overlayHotkey.shift = true;
		overlayHotkey.code = 'D';

		changed = true;
	}

	return changed;
}

void AppSettings::_SetDefaultScalingModes() {
	_scalingModes.resize(7);

	// Lanczos
	{
		auto& lanczos = _scalingModes[0];
		lanczos.name = L"Lanczos";

		auto& lanczosEffect = lanczos.effects.emplace_back();
		lanczosEffect.name = L"Lanczos";
		lanczosEffect.scalingType = ::Magpie::Core::ScalingType::Fit;
	}
	// FSR
	{
		auto& fsr = _scalingModes[1];
		fsr.name = L"FSR";

		fsr.effects.resize(2);
		auto& easu = fsr.effects[0];
		easu.name = L"FSR\\FSR_EASU";
		easu.scalingType = ::Magpie::Core::ScalingType::Fit;
		auto& rcas = fsr.effects[1];
		rcas.name = L"FSR\\FSR_RCAS";
		rcas.parameters[L"sharpness"] = 0.87f;
	}
	// FSRCNNX
	{
		auto& fsrcnnx = _scalingModes[2];
		fsrcnnx.name = L"FSRCNNX";
		fsrcnnx.effects.emplace_back().name = L"FSRCNNX\\FSRCNNX";
	}
	// ACNet
	{
		auto& acnet = _scalingModes[3];
		acnet.name = L"ACNet";
		acnet.effects.emplace_back().name = L"ACNet";
	}
	// Anime4K
	{
		auto& anime4k = _scalingModes[4];
		anime4k.name = L"Anime4K";
		anime4k.effects.emplace_back().name = L"Anime4K\\Anime4K_Upscale_Denoise_L";
	}
	// CRT-Geom
	{
		auto& crtGeom = _scalingModes[5];
		crtGeom.name = L"CRT-Geom";

		auto& crtGeomEffect = crtGeom.effects.emplace_back();
		crtGeomEffect.name = L"CRT\\CRT_Geom";
		crtGeomEffect.scalingType = ::Magpie::Core::ScalingType::Fit;
		crtGeomEffect.parameters[L"curvature"] = 0.0f;
		crtGeomEffect.parameters[L"cornerSize"] = 0.001f;
		crtGeomEffect.parameters[L"CRTGamma"] = 1.5f;
		crtGeomEffect.parameters[L"monitorGamma"] = 2.2f;
		crtGeomEffect.parameters[L"interlace"] = 0.0f;
	}
	// Integer Scale 2x
	{
		auto& integer2x = _scalingModes[6];
		integer2x.name = L"Integer Scale 2x";

		auto& nearest = integer2x.effects.emplace_back();
		nearest.name = L"Nearest";
		nearest.scalingType = ::Magpie::Core::ScalingType::Normal;
		nearest.scale = { 2.0f,2.0f };
	}

	// 降采样效果默认为 Bicubic (B=0, C=0.5)
	_downscalingEffect.name = L"Bicubic";
	_downscalingEffect.parameters[L"paramB"] = 0.0f;
	_downscalingEffect.parameters[L"paramC"] = 0.5f;
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
