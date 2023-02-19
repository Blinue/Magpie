#include "pch.h"
#include "AppSettings.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include "Logger.h"
#include "ShortcutHelper.h"
#include "Profile.h"
#include "CommonSharedConstants.h"
#include <rapidjson/prettywriter.h>
#include "AutoStartHelper.h"
#include <Magpie.Core.h>
#include "ScalingModesService.h"
#include "JsonHelper.h"
#include "ScalingMode.h"
#include "LocalizationService.h"

using namespace Magpie::Core;

namespace winrt::Magpie::App {

static constexpr uint32_t SETTINGS_VERSION = 1;

_AppSettingsData::_AppSettingsData() {}

_AppSettingsData::~_AppSettingsData() {}

// 将热键存储为 uint32_t
// 不能存储为字符串，因为某些键有相同的名称，如句号和小键盘的点
static uint32_t EncodeShortcut(const Shortcut& shortcut) noexcept {
	uint32_t value = 0;
	value |= shortcut.code;
	if (shortcut.win) {
		value |= 0x100;
	}
	if (shortcut.ctrl) {
		value |= 0x200;
	}
	if (shortcut.alt) {
		value |= 0x400;
	}
	if (shortcut.shift) {
		value |= 0x800;
	}
	return value;
}

static void DecodeShortcut(uint32_t value, Shortcut& shortcut) noexcept {
	if (value > 0xfff) {
		return;
	}

	shortcut.code = value & 0xff;
	shortcut.win = value & 0x100;
	shortcut.ctrl = value & 0x200;
	shortcut.alt = value & 0x400;
	shortcut.shift = value & 0x800;
}

static void WriteProfile(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const Profile& profile) noexcept {
	writer.StartObject();
	if (!profile.name.empty()) {
		writer.Key("name");
		writer.String(StrUtils::UTF16ToUTF8(profile.name).c_str());
		writer.Key("packaged");
		writer.Bool(profile.isPackaged);
		writer.Key("pathRule");
		writer.String(StrUtils::UTF16ToUTF8(profile.pathRule).c_str());
		writer.Key("classNameRule");
		writer.String(StrUtils::UTF16ToUTF8(profile.classNameRule).c_str());
		writer.Key("autoScale");
		writer.Bool(profile.isAutoScale);
	}

	writer.Key("scalingMode");
	writer.Int(profile.scalingMode);
	writer.Key("captureMethod");
	writer.Uint((uint32_t)profile.captureMethod);
	writer.Key("multiMonitorUsage");
	writer.Uint((uint32_t)profile.multiMonitorUsage);
	writer.Key("graphicsCard");
	writer.Int(profile.graphicsCard);

	writer.Key("disableWindowResizing");
	writer.Bool(profile.IsDisableWindowResizing());
	writer.Key("3DGameMode");
	writer.Bool(profile.Is3DGameMode());
	writer.Key("showFPS");
	writer.Bool(profile.IsShowFPS());
	writer.Key("VSync");
	writer.Bool(profile.IsVSync());
	writer.Key("tripleBuffering");
	writer.Bool(profile.IsTripleBuffering());
	writer.Key("captureTitleBar");
	writer.Bool(profile.IsCaptureTitleBar());
	writer.Key("adjustCursorSpeed");
	writer.Bool(profile.IsAdjustCursorSpeed());
	writer.Key("drawCursor");
	writer.Bool(profile.IsDrawCursor());
	writer.Key("disableDirectFlip");
	writer.Bool(profile.IsDisableDirectFlip());

	writer.Key("cursorScaling");
	writer.Uint((uint32_t)profile.cursorScaling);
	writer.Key("customCursorScaling");
	writer.Double(profile.customCursorScaling);
	writer.Key("cursorInterpolationMode");
	writer.Uint((uint32_t)profile.cursorInterpolationMode);

	writer.Key("croppingEnabled");
	writer.Bool(profile.isCroppingEnabled);
	writer.Key("cropping");
	writer.StartObject();
	writer.Key("left");
	writer.Double(profile.cropping.Left);
	writer.Key("top");
	writer.Double(profile.cropping.Top);
	writer.Key("right");
	writer.Double(profile.cropping.Right);
	writer.Key("bottom");
	writer.Double(profile.cropping.Bottom);
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
		_SetDefaultShortcuts();
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
		_SetDefaultShortcuts();
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
				_SetDefaultShortcuts();
				SaveAsync();
				return true;
			}
		}
	}

	_LoadSettings(root, settingsVersion);

	if (_SetDefaultShortcuts()) {
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

void AppSettings::Language(int value) {
	if (_language == value) {
		return;
	}

	_language = value;
	_languageChangedEvent(_language);

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

void AppSettings::SetShortcut(ShortcutAction action, const Magpie::App::Shortcut& value) {
	if (_shortcuts[(size_t)action] == value) {
		return;
	}

	_shortcuts[(size_t)action] = value;
	Logger::Get().Info(fmt::format("热键 {} 已更改为 {}", ShortcutHelper::ToString(action), StrUtils::UTF16ToUTF8(value.ToString())));
	_shortcutChangedEvent(action);

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

void AppSettings::CountdownSeconds(uint32_t value) noexcept {
	if (_countdownSeconds == value) {
		return;
	}

	_countdownSeconds = value;
	_countdownSecondsChangedEvent(value);

	SaveAsync();
}

void AppSettings::IsAlwaysRunAsAdmin(bool value) noexcept {
	if (_isAlwaysRunAsAdmin == value) {
		return;
	}

	_isAlwaysRunAsAdmin = value;
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

	writer.Key("language");
	if (_language < 0) {
		writer.String("");
	} else {
		const std::wstring& language = LocalizationService::SupportedLanguages()[_language];
		writer.String(StrUtils::UTF16ToUTF8(language).c_str());
	}

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

	writer.Key("shortcuts");
	writer.StartObject();
	writer.Key("scale");
	writer.Uint(EncodeShortcut(data._shortcuts[(size_t)ShortcutAction::Scale]));
	writer.Key("overlay");
	writer.Uint(EncodeShortcut(data._shortcuts[(size_t)ShortcutAction::Overlay]));
	writer.EndObject();

	writer.Key("autoRestore");
	writer.Bool(data._isAutoRestore);
	writer.Key("countdownSeconds");
	writer.Uint(data._countdownSeconds);
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
	writer.Key("alwaysRunAsAdmin");
	writer.Bool(data._isAlwaysRunAsAdmin);
	writer.Key("showTrayIcon");
	writer.Bool(data._isShowTrayIcon);
	writer.Key("inlineParams");
	writer.Bool(data._isInlineParams);
	writer.Key("autoCheckForUpdates");
	writer.Bool(data._isAutoCheckForUpdates);
	writer.Key("checkForPreviewUpdates");
	writer.Bool(data._isCheckForPreviewUpdates);
	writer.Key("updateCheckDate");
	writer.Int64(data._updateCheckDate.time_since_epoch().count());

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

	writer.Key("profiles");
	writer.StartArray();
	WriteProfile(writer, data._defaultProfile);
	for (const Profile& rule : data._profiles) {
		WriteProfile(writer, rule);
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

// 永远不会失败，遇到不合法的配置项时静默忽略
void AppSettings::_LoadSettings(const rapidjson::GenericObject<true, rapidjson::Value>& root, uint32_t /*version*/) {
	{
		std::wstring language;
		JsonHelper::ReadString(root, "language", language);
		if (language.empty()) {
			_language = -1;
		} else {
			std::span<const wchar_t*> languages = LocalizationService::SupportedLanguages();
			auto it = std::find(languages.begin(), languages.end(), language);
			if (it == languages.end()) {
				// 未知的语言设置，重置为使用系统设置
				_language = -1;
			} else {
				_language = int(it - languages.begin());
			}
		}
	}

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

	auto shortcutsNode = root.FindMember("shortcuts");
	if (shortcutsNode == root.MemberEnd()) {
		// v0.10.0-preview1 使用 hotkeys
		shortcutsNode= root.FindMember("hotkeys");
	}
	if (shortcutsNode != root.MemberEnd() && shortcutsNode->value.IsObject()) {
		const auto& shortcutsObj = shortcutsNode->value.GetObj();

		auto scaleNode = shortcutsObj.FindMember("scale");
		if (scaleNode != shortcutsObj.MemberEnd() && scaleNode->value.IsUint()) {
			DecodeShortcut(scaleNode->value.GetUint(), _shortcuts[(size_t)ShortcutAction::Scale]);
		}

		auto overlayNode = shortcutsObj.FindMember("overlay");
		if (overlayNode != shortcutsObj.MemberEnd() && overlayNode->value.IsUint()) {
			DecodeShortcut(overlayNode->value.GetUint(), _shortcuts[(size_t)ShortcutAction::Overlay]);
		}
	}

	JsonHelper::ReadBool(root, "autoRestore", _isAutoRestore);
	if (!JsonHelper::ReadUInt(root, "countdownSeconds", _countdownSeconds, true)) {
		// v0.10.0-preview1 使用 downCount
		JsonHelper::ReadUInt(root, "downCount", _countdownSeconds);
	}
	if (_countdownSeconds == 0 || _countdownSeconds > 5) {
		_countdownSeconds = 3;
	}
	JsonHelper::ReadBool(root, "debugMode", _isDebugMode);
	JsonHelper::ReadBool(root, "disableEffectCache", _isDisableEffectCache);
	JsonHelper::ReadBool(root, "saveEffectSources", _isSaveEffectSources);
	JsonHelper::ReadBool(root, "warningsAreErrors", _isWarningsAreErrors);
	JsonHelper::ReadBool(root, "simulateExclusiveFullscreen", _isSimulateExclusiveFullscreen);
	if (!JsonHelper::ReadBool(root, "alwaysRunAsAdmin", _isAlwaysRunAsAdmin, true)) {
		// v0.10.0-preview1 使用 alwaysRunAsElevated
		JsonHelper::ReadBool(root, "alwaysRunAsElevated", _isAlwaysRunAsAdmin);
	}
	JsonHelper::ReadBool(root, "showTrayIcon", _isShowTrayIcon);
	JsonHelper::ReadBool(root, "inlineParams", _isInlineParams);
	JsonHelper::ReadBool(root, "autoCheckForUpdates", _isAutoCheckForUpdates);
	JsonHelper::ReadBool(root, "checkForPreviewUpdates", _isCheckForPreviewUpdates);
	{
		int64_t d = 0;
		JsonHelper::ReadInt64(root, "updateCheckDate", d);

		using std::chrono::system_clock;
		_updateCheckDate = system_clock::time_point(system_clock::duration(d));
	}

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

	auto scaleProfilesNode = root.FindMember("profiles");
	if (scaleProfilesNode == root.MemberEnd()) {
		// v0.10.0-preview1 使用 scalingProfiles
		scaleProfilesNode = root.FindMember("scalingProfiles");
	}
	if (scaleProfilesNode != root.MemberEnd() && scaleProfilesNode->value.IsArray()) {
		const auto& scaleProfilesArray = scaleProfilesNode->value.GetArray();

		const rapidjson::SizeType size = scaleProfilesArray.Size();
		if (size > 0) {
			if (scaleProfilesArray[0].IsObject()) {
				// 解析默认缩放配置不会失败
				_LoadProfile(scaleProfilesArray[0].GetObj(), _defaultProfile, true);
			}

			if (size > 1) {
				_profiles.reserve((size_t)size - 1);
				for (rapidjson::SizeType i = 1; i < size; ++i) {
					if (!scaleProfilesArray[i].IsObject()) {
						continue;
					}

					Profile& rule = _profiles.emplace_back();
					if (!_LoadProfile(scaleProfilesArray[i].GetObj(), rule)) {
						_profiles.pop_back();
						continue;
					}
				}
			}
		}
	}
}

bool AppSettings::_LoadProfile(
	const rapidjson::GenericObject<true, rapidjson::Value>& profileObj,
	Profile& profile,
	bool isDefault
) {
	if (!isDefault) {
		if (!JsonHelper::ReadString(profileObj, "name", profile.name, true)) {
			return false;
		}

		{
			std::wstring_view nameView(profile.name);
			StrUtils::Trim(nameView);
			if (nameView.empty()) {
				return false;
			}
		}

		if (!JsonHelper::ReadBool(profileObj, "packaged", profile.isPackaged, true)) {
			return false;
		}

		if (!JsonHelper::ReadString(profileObj, "pathRule", profile.pathRule, true)
			|| profile.pathRule.empty()) {
			return false;
		}

		if (!JsonHelper::ReadString(profileObj, "classNameRule", profile.classNameRule, true)
			|| profile.classNameRule.empty()) {
			return false;
		}

		JsonHelper::ReadBool(profileObj, "autoScale", profile.isAutoScale);
	}

	JsonHelper::ReadInt(profileObj, "scalingMode", profile.scalingMode);
	if (profile.scalingMode < -1 || profile.scalingMode >= _scalingModes.size()) {
		profile.scalingMode = -1;
	}

	{
		uint32_t captureMethod = (uint32_t)CaptureMethod::GraphicsCapture;
		if (!JsonHelper::ReadUInt(profileObj, "captureMethod", captureMethod, true)) {
			// v0.10.0-preview1 使用 captureMode
			JsonHelper::ReadUInt(profileObj, "captureMode", captureMethod);
		}
		
		if (captureMethod > 3) {
			captureMethod = (uint32_t)CaptureMethod::GraphicsCapture;
		} else if (captureMethod == (uint32_t)CaptureMethod::DesktopDuplication) {
			// Desktop Duplication 捕获模式要求 Win10 20H1+
			if (!Win32Utils::GetOSVersion().Is20H1OrNewer()) {
				captureMethod = (uint32_t)CaptureMethod::GraphicsCapture;
			}
		}
		profile.captureMethod = (CaptureMethod)captureMethod;
	}

	{
		uint32_t multiMonitorUsage = (uint32_t)MultiMonitorUsage::Closest;
		JsonHelper::ReadUInt(profileObj, "multiMonitorUsage", multiMonitorUsage);
		if (multiMonitorUsage > 2) {
			multiMonitorUsage = (uint32_t)MultiMonitorUsage::Closest;
		}
		profile.multiMonitorUsage = (MultiMonitorUsage)multiMonitorUsage;
	}
	
	if (!JsonHelper::ReadInt(profileObj, "graphicsCard", profile.graphicsCard, true)) {
		// v0.10.0-preview1 使用 graphicsAdapter
		uint32_t graphicsAdater = 0;
		JsonHelper::ReadUInt(profileObj, "graphicsAdapter", graphicsAdater);
		profile.graphicsCard = (int)graphicsAdater - 1;
	}

	JsonHelper::ReadBoolFlag(profileObj, "disableWindowResizing", MagFlags::DisableWindowResizing, profile.flags);
	JsonHelper::ReadBoolFlag(profileObj, "3DGameMode", MagFlags::Is3DGameMode, profile.flags);
	JsonHelper::ReadBoolFlag(profileObj, "showFPS", MagFlags::ShowFPS, profile.flags);
	JsonHelper::ReadBoolFlag(profileObj, "VSync", MagFlags::VSync, profile.flags);
	JsonHelper::ReadBoolFlag(profileObj, "tripleBuffering", MagFlags::TripleBuffering, profile.flags);
	if (!JsonHelper::ReadBoolFlag(profileObj, "captureTitleBar", MagFlags::CaptureTitleBar, profile.flags, true)) {
		// v0.10.0-preview1 使用 reserveTitleBar
		JsonHelper::ReadBoolFlag(profileObj, "reserveTitleBar", MagFlags::CaptureTitleBar, profile.flags);
	}
	JsonHelper::ReadBoolFlag(profileObj, "adjustCursorSpeed", MagFlags::AdjustCursorSpeed, profile.flags);
	JsonHelper::ReadBoolFlag(profileObj, "drawCursor", MagFlags::DrawCursor, profile.flags);
	JsonHelper::ReadBoolFlag(profileObj, "disableDirectFlip", MagFlags::DisableDirectFlip, profile.flags);

	{
		uint32_t cursorScaling = (uint32_t)CursorScaling::NoScaling;
		JsonHelper::ReadUInt(profileObj, "cursorScaling", cursorScaling);
		if (cursorScaling > 7) {
			cursorScaling = (uint32_t)CursorScaling::NoScaling;
		}
		profile.cursorScaling = (CursorScaling)cursorScaling;
	}
	
	JsonHelper::ReadFloat(profileObj, "customCursorScaling", profile.customCursorScaling);
	if (profile.customCursorScaling < 0) {
		profile.customCursorScaling = 1.0f;
	}

	{
		uint32_t cursorInterpolationMode = (uint32_t)CursorInterpolationMode::NearestNeighbor;
		JsonHelper::ReadUInt(profileObj, "cursorInterpolationMode", (uint32_t&)profile.cursorInterpolationMode);
		if (cursorInterpolationMode > 1) {
			cursorInterpolationMode = (uint32_t)CursorInterpolationMode::NearestNeighbor;
		}
		profile.cursorInterpolationMode = (CursorInterpolationMode)cursorInterpolationMode;
	}

	JsonHelper::ReadBool(profileObj, "croppingEnabled", profile.isCroppingEnabled);

	auto croppingNode = profileObj.FindMember("cropping");
	if (croppingNode != profileObj.MemberEnd() && croppingNode->value.IsObject()) {
		const auto& croppingObj = croppingNode->value.GetObj();

		if (!JsonHelper::ReadFloat(croppingObj, "left", profile.cropping.Left, true)
			|| profile.cropping.Left < 0
			|| !JsonHelper::ReadFloat(croppingObj, "top", profile.cropping.Top, true)
			|| profile.cropping.Top < 0
			|| !JsonHelper::ReadFloat(croppingObj, "right", profile.cropping.Right, true)
			|| profile.cropping.Right < 0
			|| !JsonHelper::ReadFloat(croppingObj, "bottom", profile.cropping.Bottom, true)
			|| profile.cropping.Bottom < 0
		) {
			profile.cropping = {};
		}
	}

	return true;
}

bool AppSettings::_SetDefaultShortcuts() {
	bool changed = false;

	Shortcut& scaleShortcut = _shortcuts[(size_t)ShortcutAction::Scale];
	if (scaleShortcut.IsEmpty()) {
		scaleShortcut.win = true;
		scaleShortcut.shift = true;
		scaleShortcut.code = 'A';

		changed = true;
	}

	Shortcut& overlayShortcut = _shortcuts[(size_t)ShortcutAction::Overlay];
	if (overlayShortcut.IsEmpty()) {
		overlayShortcut.win = true;
		overlayShortcut.shift = true;
		overlayShortcut.code = 'D';

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
