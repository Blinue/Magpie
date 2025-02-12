#include "pch.h"
#include "AppSettings.h"
#include "StrHelper.h"
#include "Win32Helper.h"
#include "Logger.h"
#include "ShortcutHelper.h"
#include "Profile.h"
#include "CommonSharedConstants.h"
#include <rapidjson/prettywriter.h>
#include "AutoStartHelper.h"
#include "ScalingModesService.h"
#include "JsonHelper.h"
#include "ScalingMode.h"
#include "LocalizationService.h"
#include <ShellScalingApi.h>
#include "resource.h"
#include "App.h"
#include "MainWindow.h"
#include <ShlObj.h>

using namespace winrt;
using namespace winrt::Magpie;  

namespace Magpie {

// 如果配置文件和已发布的正式版本不再兼容，应提高此版本号
static constexpr uint32_t CONFIG_VERSION = 3;

_AppSettingsData::_AppSettingsData() {}

_AppSettingsData::~_AppSettingsData() {}

// 将热键存储为 uint32_t
// 不能存储为字符串，因为某些键的字符相同，如句号和小键盘的点
static uint32_t EncodeShortcut(const Shortcut& shortcut) noexcept {
	uint32_t value = shortcut.code;
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
		writer.String(StrHelper::UTF16ToUTF8(profile.name).c_str());
		writer.Key("packaged");
		writer.Bool(profile.isPackaged);
		writer.Key("pathRule");
		writer.String(StrHelper::UTF16ToUTF8(profile.pathRule).c_str());
		writer.Key("classNameRule");
		writer.String(StrHelper::UTF16ToUTF8(profile.classNameRule).c_str());
		writer.Key("launcherPath");
		writer.String(StrHelper::UTF16ToUTF8(profile.launcherPath).c_str());
		writer.Key("autoScale");
		writer.Bool(profile.isAutoScale);
		writer.Key("launchParameters");
		writer.String(StrHelper::UTF16ToUTF8(profile.launchParameters).c_str());
	}

	writer.Key("scalingMode");
	writer.Int(profile.scalingMode);
	writer.Key("captureMethod");
	writer.Uint((uint32_t)profile.captureMethod);
	writer.Key("multiMonitorUsage");
	writer.Uint((uint32_t)profile.multiMonitorUsage);

	writer.Key("graphicsCardId");
	writer.StartObject();
	writer.Key("idx");
	writer.Int(profile.graphicsCardId.idx);
	writer.Key("vendorId");
	writer.Uint(profile.graphicsCardId.vendorId);
	writer.Key("deviceId");
	writer.Uint(profile.graphicsCardId.deviceId);
	writer.EndObject();
	writer.Key("frameRateLimiterEnabled");
	writer.Bool(profile.isFrameRateLimiterEnabled);
	writer.Key("maxFrameRate");
	writer.Double(profile.maxFrameRate);

	writer.Key("disableWindowResizing");
	writer.Bool(profile.IsWindowResizingDisabled());
	writer.Key("3DGameMode");
	writer.Bool(profile.Is3DGameMode());
	writer.Key("showFPS");
	writer.Bool(profile.IsShowFPS());
	writer.Key("captureTitleBar");
	writer.Bool(profile.IsCaptureTitleBar());
	writer.Key("adjustCursorSpeed");
	writer.Bool(profile.IsAdjustCursorSpeed());
	writer.Key("drawCursor");
	writer.Bool(profile.IsDrawCursor());
	writer.Key("disableDirectFlip");
	writer.Bool(profile.IsDirectFlipDisabled());

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
	LoadIconMetric(hInst, MAKEINTRESOURCE(IDI_APP), large ? LIM_LARGE : LIM_SMALL, &hIconApp);
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
		// GetModuleHandle 获取 exe 文件的句柄
		HINSTANCE hInst = GetModuleHandle(nullptr);
		ReplaceIcon(hInst, hWnd, true);
		ReplaceIcon(hInst, hWnd, false);

		// 删除标题栏中的图标
		INT_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
		SetWindowLongPtr(hWnd, GWL_STYLE, style & ~WS_SYSMENU);
	}

	return S_OK;
}

static void ShowErrorMessage(const wchar_t* mainInstruction, const wchar_t* content) noexcept {
	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	const hstring errorStr = resourceLoader.GetString(L"AppSettings_Dialog_Error");
	const hstring exitStr = resourceLoader.GetString(L"AppSettings_Dialog_Exit");

	TASKDIALOG_BUTTON button{ IDCANCEL, exitStr.c_str() };
	TASKDIALOGCONFIG tdc{
		.cbSize = sizeof(TASKDIALOGCONFIG),
		.dwFlags = TDF_SIZE_TO_CONTENT,
		.pszWindowTitle = errorStr.c_str(),
		.pszMainIcon = TD_ERROR_ICON,
		.pszMainInstruction = mainInstruction,
		.pszContent = content,
		.cButtons = 1,
		.pButtons = &button,
		.pfCallback = TaskDialogCallback
	};
	TaskDialogIndirect(&tdc, nullptr, nullptr, nullptr);
}

AppSettings::~AppSettings() {}

bool AppSettings::Initialize() noexcept {
	Logger& logger = Logger::Get();

	// 若程序所在目录存在配置文件则为便携模式
	_isPortableMode = Win32Helper::FileExists(StrHelper::Concat(
		CommonSharedConstants::CONFIG_DIR, CommonSharedConstants::CONFIG_FILENAME).c_str());

	std::wstring existingConfigPath;
	if (!_UpdateConfigPath(&existingConfigPath)) {
		logger.Error("_UpdateConfigPath 失败");
		return false;
	}

	logger.Info(StrHelper::Concat("便携模式: ", _isPortableMode ? "是" : "否"));

	if (existingConfigPath.empty()) {
		logger.Info("不存在配置文件");
		_SetDefaultScalingModes();
		_SetDefaultShortcuts();
		SaveAsync();
		return true;
	}

	// 此时 ResourceLoader 使用“首选语言”
	
	std::string configText;
	if (!Win32Helper::ReadTextFile(existingConfigPath.c_str(), configText)) {
		logger.Error("读取配置文件失败");
		ResourceLoader resourceLoader =
			ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
		hstring title = resourceLoader.GetString(L"AppSettings_ErrorDialog_ReadFailed");
		hstring content = resourceLoader.GetString(L"AppSettings_ErrorDialog_ConfigLocation");
		ShowErrorMessage(title.c_str(),
			fmt::format(fmt::runtime(std::wstring_view(content)), existingConfigPath).c_str());
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
		Logger::Get().Error(fmt::format("解析配置失败\n\t错误码: {}", (int)doc.GetParseError()));
		ResourceLoader resourceLoader =
			ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
		hstring title = resourceLoader.GetString(L"AppSettings_ErrorDialog_NotValidJson");
		hstring content = resourceLoader.GetString(L"AppSettings_ErrorDialog_ConfigLocation");
		ShowErrorMessage(title.c_str(),
			fmt::format(fmt::runtime(std::wstring_view(content)), existingConfigPath).c_str());
		return false;
	}

	if (!doc.IsObject()) {
		Logger::Get().Error("配置文件根元素不是 Object");
		ResourceLoader resourceLoader =
			ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
		hstring title = resourceLoader.GetString(L"AppSettings_ErrorDialog_ParseFailed");
		hstring content = resourceLoader.GetString(L"AppSettings_ErrorDialog_ConfigLocation");
		ShowErrorMessage(title.c_str(),
			fmt::format(fmt::runtime(std::wstring_view(content)), existingConfigPath).c_str());
		return false;
	}

	_LoadSettings(((const rapidjson::Document&)doc).GetObj());

	// 迁移旧版配置后立刻保存，_SetDefaultShortcuts 用于确保快捷键不为空
	if (_SetDefaultShortcuts() || !Win32Helper::FileExists(_configPath.c_str())) {
		SaveAsync();
	}

	return true;
}

bool AppSettings::Save() noexcept {
	_UpdateWindowPlacement();
	return _Save(*this);
}

fire_and_forget AppSettings::SaveAsync() noexcept {
	_UpdateWindowPlacement();

	// 拷贝当前配置
	_AppSettingsData data = *this;
	co_await resume_background();

	_Save(data);
}

void AppSettings::IsPortableMode(bool value) noexcept {
	if (_isPortableMode == value) {
		return;
	}

	if (!value) {
		// 关闭便携模式需删除本地配置文件
		if (!DeleteFile(StrHelper::Concat(_configDir, CommonSharedConstants::CONFIG_FILENAME).c_str())) {
			if (GetLastError() != ERROR_FILE_NOT_FOUND) {
				Logger::Get().Win32Error("删除本地配置文件失败");
				return;
			}
		}
	}

	_isPortableMode = value;

	if (_UpdateConfigPath()) {
		Logger::Get().Info(value ? "已开启便携模式" : "已关闭便携模式");
		SaveAsync();
	} else {
		Logger::Get().Error(value ? "开启便携模式失败" : "关闭便携模式失败");
		_isPortableMode = !value;
	}
}

void AppSettings::Language(int value) {
	if (_language == value) {
		return;
	}

	_language = value;
	SaveAsync();
}

void AppSettings::Theme(AppTheme value) {
	if (_theme == value) {
		return;
	}

	_theme = value;
	ThemeChanged.Invoke(value);

	SaveAsync();
}

void AppSettings::SetShortcut(ShortcutAction action, const Shortcut& value) {
	if (_shortcuts[(size_t)action] == value) {
		return;
	}

	_shortcuts[(size_t)action] = value;
	Logger::Get().Info(fmt::format("热键 {} 已更改为 {}", ShortcutHelper::ToString(action), StrHelper::UTF16ToUTF8(value.ToString())));
	ShortcutChanged.Invoke(action);

	SaveAsync();
}

void AppSettings::CountdownSeconds(uint32_t value) noexcept {
	if (_countdownSeconds == value) {
		return;
	}

	_countdownSeconds = value;
	CountdownSecondsChanged.Invoke(value);

	SaveAsync();
}

void AppSettings::IsDeveloperMode(bool value) noexcept {
	_isDeveloperMode = value;
	if (!value) {
		// 关闭开发者模式则禁用所有开发者选项
		_isDebugMode = false;
		_isBenchmarkMode = false;
		_isEffectCacheDisabled = false;
		_isFontCacheDisabled = false;
		_isSaveEffectSources = false;
		_isWarningsAreErrors = false;
		_duplicateFrameDetectionMode = DuplicateFrameDetectionMode::Dynamic;
		_isStatisticsForDynamicDetectionEnabled = false;
		_isFP16Disabled = false;
	}

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
		AutoStartHelper::EnableAutoStart(value, _isShowNotifyIcon ? arguments.c_str() : nullptr);
	}

	SaveAsync();
}

void AppSettings::IsShowNotifyIcon(bool value) noexcept {
	if (_isShowNotifyIcon == value) {
		return;
	}

	_isShowNotifyIcon = value;
	IsShowNotifyIconChanged.Invoke(value);

	SaveAsync();
}

void AppSettings::_UpdateWindowPlacement() noexcept {
	const HWND hwndMain = implementation::App::Get().MainWindow().Handle();;
	if (!hwndMain) {
		return;
	}

	WINDOWPLACEMENT wp{ sizeof(wp) };
	if (!GetWindowPlacement(hwndMain, &wp)) {
		Logger::Get().Win32Error("GetWindowPlacement 失败");
		return;
	}

	_mainWindowCenter = {
		(wp.rcNormalPosition.left + wp.rcNormalPosition.right) / 2.0f,
		(wp.rcNormalPosition.top + wp.rcNormalPosition.bottom) / 2.0f
	};

	const float dpiFactor = GetDpiForWindow(hwndMain) / float(USER_DEFAULT_SCREEN_DPI);
	_mainWindowSizeInDips = {
		(wp.rcNormalPosition.right - wp.rcNormalPosition.left) / dpiFactor,
		(wp.rcNormalPosition.bottom - wp.rcNormalPosition.top) / dpiFactor,
	};

	_isMainWindowMaximized = wp.showCmd == SW_MAXIMIZE;
}

bool AppSettings::_Save(const _AppSettingsData& data) noexcept {
	if (!Win32Helper::CreateDir(data._configDir, true)) {
		Logger::Get().Win32Error("创建配置文件夹失败");
		return false;
	}

	rapidjson::StringBuffer json;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(json);
	writer.StartObject();

	writer.Key("language");
	if (_language < 0) {
		writer.String("");
	} else {
		const wchar_t* language = LocalizationService::SupportedLanguages()[_language];
		writer.String(StrHelper::UTF16ToUTF8(language).c_str());
	}

	writer.Key("theme");
	writer.Uint((uint32_t)data._theme);

	writer.Key("windowPos");
	writer.StartObject();
	writer.Key("centerX");
	writer.Double(data._mainWindowCenter.X);
	writer.Key("centerY");
	writer.Double(data._mainWindowCenter.Y);
	writer.Key("width");
	writer.Double(data._mainWindowSizeInDips.Width);
	writer.Key("height");
	writer.Double(data._mainWindowSizeInDips.Height);
	writer.Key("maximized");
	writer.Bool(data._isMainWindowMaximized);
	writer.EndObject();

	writer.Key("shortcuts");
	writer.StartObject();
	writer.Key("scale");
	writer.Uint(EncodeShortcut(data._shortcuts[(size_t)ShortcutAction::Scale]));
	writer.Key("windowedModeScale");
	writer.Uint(EncodeShortcut(data._shortcuts[(size_t)ShortcutAction::WindowedModeScale]));
	writer.Key("overlay");
	writer.Uint(EncodeShortcut(data._shortcuts[(size_t)ShortcutAction::Overlay]));
	writer.EndObject();

	writer.Key("countdownSeconds");
	writer.Uint(data._countdownSeconds);
	writer.Key("developerMode");
	writer.Bool(data._isDeveloperMode);
	writer.Key("debugMode");
	writer.Bool(data._isDebugMode);
	writer.Key("benchmarkMode");
	writer.Bool(data._isBenchmarkMode);
	writer.Key("disableEffectCache");
	writer.Bool(data._isEffectCacheDisabled);
	writer.Key("disableFontCache");
	writer.Bool(data._isFontCacheDisabled);
	writer.Key("saveEffectSources");
	writer.Bool(data._isSaveEffectSources);
	writer.Key("warningsAreErrors");
	writer.Bool(data._isWarningsAreErrors);
	writer.Key("allowScalingMaximized");
	writer.Bool(data._isAllowScalingMaximized);
	writer.Key("simulateExclusiveFullscreen");
	writer.Bool(data._isSimulateExclusiveFullscreen);
	writer.Key("alwaysRunAsAdmin");
	writer.Bool(data._isAlwaysRunAsAdmin);
	writer.Key("showNotifyIcon");
	writer.Bool(data._isShowNotifyIcon);
	writer.Key("inlineParams");
	writer.Bool(data._isInlineParams);
	writer.Key("autoCheckForUpdates");
	writer.Bool(data._isAutoCheckForUpdates);
	writer.Key("checkForPreviewUpdates");
	writer.Bool(data._isCheckForPreviewUpdates);
	writer.Key("updateCheckDate");
	writer.Int64(data._updateCheckDate.time_since_epoch().count());
	writer.Key("duplicateFrameDetectionMode");
	writer.Uint((uint32_t)data._duplicateFrameDetectionMode);
	writer.Key("enableStatisticsForDynamicDetection");
	writer.Bool(data._isStatisticsForDynamicDetectionEnabled);
	writer.Key("minFrameRate");
	writer.Double(data._minFrameRate);
	writer.Key("disableFP16");
	writer.Bool(data._isFP16Disabled);

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
	auto lock = _saveLock.lock_exclusive();
	if (!Win32Helper::WriteTextFile(data._configPath.c_str(), { json.GetString(), json.GetLength() })) {
		Logger::Get().Error("保存配置失败");
		return false;
	}

	return true;
}

// 永远不会失败，遇到不合法的配置项时静默忽略
void AppSettings::_LoadSettings(const rapidjson::GenericObject<true, rapidjson::Value>& root) noexcept {
	{
		std::wstring language;
		JsonHelper::ReadString(root, "language", language);
		if (language.empty()) {
			_language = -1;
		} else {
			StrHelper::ToLowerCase(language);
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

	{
		uint32_t theme = (uint32_t)AppTheme::System;
		JsonHelper::ReadUInt(root, "theme", theme);
		if (theme <= 2) {
			_theme = (AppTheme)theme;
		} else {
			_theme = AppTheme::System;
		}
	}

	auto windowPosNode = root.FindMember("windowPos");
	if (windowPosNode != root.MemberEnd() && windowPosNode->value.IsObject()) {
		const auto& windowPosObj = windowPosNode->value.GetObj();

		Point center{};
		Size size{};
		if (JsonHelper::ReadFloat(windowPosObj, "centerX", center.X, true) &&
			JsonHelper::ReadFloat(windowPosObj, "centerY", center.Y, true) &&
			JsonHelper::ReadFloat(windowPosObj, "width", size.Width, true) &&
			JsonHelper::ReadFloat(windowPosObj, "height", size.Height, true)) {
			_mainWindowCenter = center;
			_mainWindowSizeInDips = size;
		} else {
			// 尽最大努力和旧版本兼容
			int x = 0;
			int y = 0;
			uint32_t width = 0;
			uint32_t height = 0;
			if (JsonHelper::ReadInt(windowPosObj, "x", x, true) &&
				JsonHelper::ReadInt(windowPosObj, "y", y, true) &&
				JsonHelper::ReadUInt(windowPosObj, "width", width, true) &&
				JsonHelper::ReadUInt(windowPosObj, "height", height, true)) {
				_mainWindowCenter = {
					x + width / 2.0f,
					y + height / 2.0f
				};

				// 如果窗口位置不存在屏幕则使用主屏幕的缩放，猜错的后果仅是窗口尺寸错误，
				// 无论如何原始缩放信息已经丢失。
				const HMONITOR hMon = MonitorFromPoint(
					{ std::lroundf(_mainWindowCenter.X), std::lroundf(_mainWindowCenter.Y) },
					MONITOR_DEFAULTTOPRIMARY
				);

				UINT dpi = USER_DEFAULT_SCREEN_DPI;
				GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpi, &dpi);
				const float dpiFactor = dpi / float(USER_DEFAULT_SCREEN_DPI);
				_mainWindowSizeInDips = {
					width / dpiFactor,
					height / dpiFactor
				};
			}
		}

		JsonHelper::ReadBool(windowPosObj, "maximized", _isMainWindowMaximized);
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

		auto windowedModeScaleNode = shortcutsObj.FindMember("windowedModeScale");
		if (windowedModeScaleNode != shortcutsObj.MemberEnd() && windowedModeScaleNode->value.IsUint()) {
			DecodeShortcut(windowedModeScaleNode->value.GetUint(), _shortcuts[(size_t)ShortcutAction::WindowedModeScale]);
		}

		auto overlayNode = shortcutsObj.FindMember("overlay");
		if (overlayNode != shortcutsObj.MemberEnd() && overlayNode->value.IsUint()) {
			DecodeShortcut(overlayNode->value.GetUint(), _shortcuts[(size_t)ShortcutAction::Overlay]);
		}
	}

	if (!JsonHelper::ReadUInt(root, "countdownSeconds", _countdownSeconds, true)) {
		// v0.10.0-preview1 使用 downCount
		JsonHelper::ReadUInt(root, "downCount", _countdownSeconds);
	}
	if (_countdownSeconds == 0 || _countdownSeconds > 5) {
		_countdownSeconds = 3;
	}
	JsonHelper::ReadBool(root, "developerMode", _isDeveloperMode);
	JsonHelper::ReadBool(root, "debugMode", _isDebugMode);
	JsonHelper::ReadBool(root, "benchmarkMode", _isBenchmarkMode);
	JsonHelper::ReadBool(root, "disableEffectCache", _isEffectCacheDisabled);
	JsonHelper::ReadBool(root, "disableFontCache", _isFontCacheDisabled);
	JsonHelper::ReadBool(root, "saveEffectSources", _isSaveEffectSources);
	JsonHelper::ReadBool(root, "warningsAreErrors", _isWarningsAreErrors);
	JsonHelper::ReadBool(root, "allowScalingMaximized", _isAllowScalingMaximized);
	JsonHelper::ReadBool(root, "simulateExclusiveFullscreen", _isSimulateExclusiveFullscreen);
	if (!JsonHelper::ReadBool(root, "alwaysRunAsAdmin", _isAlwaysRunAsAdmin, true)) {
		// v0.10.0-preview1 使用 alwaysRunAsElevated
		JsonHelper::ReadBool(root, "alwaysRunAsElevated", _isAlwaysRunAsAdmin);
	}
	if (!JsonHelper::ReadBool(root, "showNotifyIcon", _isShowNotifyIcon, true)) {
		// v0.10 使用 showTrayIcon
		JsonHelper::ReadBool(root, "showTrayIcon", _isShowNotifyIcon);
	}
	JsonHelper::ReadBool(root, "inlineParams", _isInlineParams);
	JsonHelper::ReadBool(root, "autoCheckForUpdates", _isAutoCheckForUpdates);
	JsonHelper::ReadBool(root, "checkForPreviewUpdates", _isCheckForPreviewUpdates);
	{
		int64_t d = 0;
		JsonHelper::ReadInt64(root, "updateCheckDate", d);

		using std::chrono::system_clock;
		_updateCheckDate = system_clock::time_point(system_clock::duration(d));
	}
	{
		uint32_t duplicateFrameDetectionMode = (uint32_t)DuplicateFrameDetectionMode::Dynamic;
		JsonHelper::ReadUInt(root, "duplicateFrameDetectionMode", duplicateFrameDetectionMode);
		if (duplicateFrameDetectionMode > 2) {
			duplicateFrameDetectionMode = (uint32_t)DuplicateFrameDetectionMode::Dynamic;
		}
		_duplicateFrameDetectionMode = (::Magpie::DuplicateFrameDetectionMode)duplicateFrameDetectionMode;
	}
	JsonHelper::ReadBool(root, "enableStatisticsForDynamicDetection", _isStatisticsForDynamicDetectionEnabled);
	JsonHelper::ReadFloat(root, "minFrameRate", _minFrameRate);
	JsonHelper::ReadBool(root, "disableFP16", _isFP16Disabled);

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
) const noexcept {
	if (!isDefault) {
		if (!JsonHelper::ReadString(profileObj, "name", profile.name, true)) {
			return false;
		}

		{
			std::wstring_view nameView(profile.name);
			StrHelper::Trim(nameView);
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

		JsonHelper::ReadString(profileObj, "launcherPath", profile.launcherPath);
		// 将旧版本的相对路径转换为绝对路径
		if (PathIsRelative(profile.launcherPath.c_str())) {
			size_t delimPos = profile.pathRule.find_last_of(L'\\');
			if (delimPos != std::wstring::npos) {
				wil::unique_hlocal_string combinedPath;
				if (SUCCEEDED(PathAllocCombine(
					profile.pathRule.substr(0, delimPos).c_str(),
					profile.launcherPath.c_str(),
					PATHCCH_ALLOW_LONG_PATHS,
					combinedPath.put()
				))) {
					profile.launcherPath.assign(combinedPath.get());
				}
			}
		}

		JsonHelper::ReadBool(profileObj, "autoScale", profile.isAutoScale);
		JsonHelper::ReadString(profileObj, "launchParameters", profile.launchParameters);
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
			if (!Win32Helper::GetOSVersion().Is20H1OrNewer()) {
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
	
	{
		auto graphicsCardIdNode = profileObj.FindMember("graphicsCardId");
		if (graphicsCardIdNode == profileObj.end()) {
			// v0.10 和 v0.11 只使用索引
			int graphicsCardIdx = -1;
			if (!JsonHelper::ReadInt(profileObj, "graphicsCard", graphicsCardIdx, true)) {
				// v0.10.0-preview1 使用 graphicsAdapter
				uint32_t graphicsAdater = 0;
				JsonHelper::ReadUInt(profileObj, "graphicsAdapter", graphicsAdater);
				graphicsCardIdx = (int)graphicsAdater - 1;
			}

			// 稍后由 ProfileService 设置 vendorId 和 deviceId
			profile.graphicsCardId.idx = graphicsCardIdx;
		} else if (graphicsCardIdNode->value.IsObject()) {
			auto graphicsCardIdObj = graphicsCardIdNode->value.GetObj();

			auto idxNode = graphicsCardIdObj.FindMember("idx");
			if (idxNode != graphicsCardIdObj.end() && idxNode->value.IsInt()) {
				profile.graphicsCardId.idx = idxNode->value.GetInt();
			}

			auto vendorIdNode = graphicsCardIdObj.FindMember("vendorId");
			if (vendorIdNode != graphicsCardIdObj.end() && vendorIdNode->value.IsUint()) {
				profile.graphicsCardId.vendorId = vendorIdNode->value.GetUint();
			}

			auto deviceIdNode = graphicsCardIdObj.FindMember("deviceId");
			if (deviceIdNode != graphicsCardIdObj.end() && deviceIdNode->value.IsUint()) {
				profile.graphicsCardId.deviceId = deviceIdNode->value.GetUint();
			}
		}
	}

	JsonHelper::ReadBool(profileObj, "frameRateLimiterEnabled", profile.isFrameRateLimiterEnabled);
	JsonHelper::ReadFloat(profileObj, "maxFrameRate", profile.maxFrameRate);
	if (profile.maxFrameRate < 10.0f || profile.maxFrameRate > 1000.0f) {
		profile.maxFrameRate = 60.0f;
	}

	JsonHelper::ReadBoolFlag(profileObj, "disableWindowResizing", ScalingFlags::DisableWindowResizing, profile.scalingFlags);
	JsonHelper::ReadBoolFlag(profileObj, "3DGameMode", ScalingFlags::Is3DGameMode, profile.scalingFlags);
	JsonHelper::ReadBoolFlag(profileObj, "showFPS", ScalingFlags::ShowFPS, profile.scalingFlags);
	if (!JsonHelper::ReadBoolFlag(profileObj, "captureTitleBar", ScalingFlags::CaptureTitleBar, profile.scalingFlags, true)) {
		// v0.10.0-preview1 使用 reserveTitleBar
		JsonHelper::ReadBoolFlag(profileObj, "reserveTitleBar", ScalingFlags::CaptureTitleBar, profile.scalingFlags);
	}
	JsonHelper::ReadBoolFlag(profileObj, "adjustCursorSpeed", ScalingFlags::AdjustCursorSpeed, profile.scalingFlags);
	JsonHelper::ReadBoolFlag(profileObj, "drawCursor", ScalingFlags::DrawCursor, profile.scalingFlags);
	JsonHelper::ReadBoolFlag(profileObj, "disableDirectFlip", ScalingFlags::DisableDirectFlip, profile.scalingFlags);

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
		JsonHelper::ReadUInt(profileObj, "cursorInterpolationMode", cursorInterpolationMode);
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

bool AppSettings::_SetDefaultShortcuts() noexcept {
	bool changed = false;

	Shortcut& scaleShortcut = _shortcuts[(size_t)ShortcutAction::Scale];
	if (scaleShortcut.IsEmpty()) {
		scaleShortcut.win = true;
		scaleShortcut.shift = true;
		scaleShortcut.code = 'A';

		changed = true;
	}

	Shortcut& windowedModeScaleShortcut = _shortcuts[(size_t)ShortcutAction::WindowedModeScale];
	if (windowedModeScaleShortcut.IsEmpty()) {
		windowedModeScaleShortcut.win = true;
		windowedModeScaleShortcut.shift = true;
		windowedModeScaleShortcut.code = 'Q';

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

void AppSettings::_SetDefaultScalingModes() noexcept {
	_scalingModes.resize(7);

	// Lanczos
	{
		auto& lanczos = _scalingModes[0];
		lanczos.name = L"Lanczos";

		auto& lanczosEffect = lanczos.effects.emplace_back();
		lanczosEffect.name = L"Lanczos";
		lanczosEffect.scalingType = ::Magpie::ScalingType::Fit;
	}
	// FSR
	{
		auto& fsr = _scalingModes[1];
		fsr.name = L"FSR";

		fsr.effects.resize(2);
		auto& easu = fsr.effects[0];
		easu.name = L"FSR\\FSR_EASU";
		easu.scalingType = ::Magpie::ScalingType::Fit;
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
		crtGeomEffect.scalingType = ::Magpie::ScalingType::Fit;
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
		nearest.scalingType = ::Magpie::ScalingType::Normal;
		nearest.scale = { 2.0f,2.0f };
	}

	// 全局缩放模式默认为 Lanczos
	_defaultProfile.scalingMode = 0;
}

static std::wstring FindOldConfig(const wchar_t* localAppDataDir) noexcept {
	for (uint32_t version = CONFIG_VERSION - 1; version >= 2; --version) {
		std::wstring oldConfigPath = fmt::format(
			L"{}\\Magpie\\{}v{}\\{}",
			localAppDataDir,
			CommonSharedConstants::CONFIG_DIR,
			version,
			CommonSharedConstants::CONFIG_FILENAME
		);

		if (Win32Helper::FileExists(oldConfigPath.c_str())) {
			return oldConfigPath;
		}
	}

	// v1 版本的配置文件不在子目录中
	std::wstring v1ConfigPath = StrHelper::Concat(
		localAppDataDir,
		L"\\Magpie\\",
		CommonSharedConstants::CONFIG_DIR,
		CommonSharedConstants::CONFIG_FILENAME
	);

	if (Win32Helper::FileExists(v1ConfigPath.c_str())) {
		return v1ConfigPath;
	}

	return {};
}

bool AppSettings::_UpdateConfigPath(std::wstring* existingConfigPath) noexcept {
	if (_isPortableMode) {
		HRESULT hr = wil::GetFullPathNameW(CommonSharedConstants::CONFIG_DIR, _configDir);
		if (FAILED(hr)) {
			Logger::Get().ComError("GetFullPathNameW 失败", hr);
			return false;
		}

		_configPath = _configDir + CommonSharedConstants::CONFIG_FILENAME;

		if (existingConfigPath) {
			if (Win32Helper::FileExists(_configPath.c_str())) {
				*existingConfigPath = _configPath;
			}
		}
	} else {
		wil::unique_cotaskmem_string localAppDataDir;
		HRESULT hr = SHGetKnownFolderPath(
			FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, localAppDataDir.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("SHGetKnownFolderPath 失败", hr);
			return false;
		}

		_configDir = fmt::format(L"{}\\Magpie\\{}v{}\\",
			localAppDataDir.get(), CommonSharedConstants::CONFIG_DIR, CONFIG_VERSION);
		_configPath = _configDir + CommonSharedConstants::CONFIG_FILENAME;

		if (existingConfigPath) {
			if (Win32Helper::FileExists(_configPath.c_str())) {
				*existingConfigPath = _configPath;
			} else {
				// 查找旧版本配置文件
				*existingConfigPath = FindOldConfig(localAppDataDir.get());
			}
		}
	}

	// 确保配置文件夹存在
	if (!Win32Helper::CreateDir(_configDir, true)) {
		Logger::Get().Win32Error("创建配置文件夹失败");
		return false;
	}

	return true;
}

}
