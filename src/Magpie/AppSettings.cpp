#include "pch.h"
#include "AppSettings.h"
#include "App.h"
#include "AutoStartHelper.h"
#include "CommonSharedConstants.h"
#include "JsonHelper.h"
#include "LocalizationService.h"
#include "Logger.h"
#include "MainWindow.h"
#include "Profile.h"
#include "resource.h"
#include "ScalingMode.h"
#include "ScalingModesService.h"
#include "ShortcutHelper.h"
#include "StrHelper.h"
#include "Win32Helper.h"
#include <rapidjson/prettywriter.h>
#include <ShellScalingApi.h>
#include <ShlObj.h>

using namespace winrt::Magpie;

namespace Magpie {

// 如果配置文件和已发布的正式版本不再兼容，应提高此版本号
static constexpr uint32_t CONFIG_VERSION = 4;

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
		writer.String(StrHelper::UTF16ToUTF8(profile.launcherPath.native()).c_str());
		writer.Key("autoScale");
		writer.Uint((uint32_t)profile.autoScale);
		writer.Key("launchParameters");
		writer.String(StrHelper::UTF16ToUTF8(profile.launchParameters).c_str());
	}

	writer.Key("scalingMode");
	writer.Int(profile.scalingMode);
	writer.Key("captureMethod");
	writer.Uint((uint32_t)profile.captureMethod);
	writer.Key("multiMonitorUsage");
	writer.Uint((uint32_t)profile.multiMonitorUsage);

	writer.Key("initialWindowedScaleFactor");
	writer.Uint((uint32_t)profile.initialWindowedScaleFactor);
	writer.Key("customInitialWindowedScaleFactor");
	writer.Double(profile.customInitialWindowedScaleFactor);

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

	writer.Key("3DGameMode");
	writer.Bool(profile.Is3DGameMode());
	writer.Key("captureTitleBar");
	writer.Bool(profile.IsCaptureTitleBar());
	writer.Key("adjustCursorSpeed");
	writer.Bool(profile.IsAdjustCursorSpeed());
	writer.Key("disableDirectFlip");
	writer.Bool(profile.IsDirectFlipDisabled());

	writer.Key("cursorScaling");
	writer.Uint((uint32_t)profile.cursorScaling);
	writer.Key("customCursorScaling");
	writer.Double(profile.customCursorScaleFactor);
	writer.Key("cursorInterpolationMode");
	writer.Uint((uint32_t)profile.cursorInterpolationMode);
	writer.Key("autoHideCursorEnabled");
	writer.Bool(profile.isAutoHideCursorEnabled);
	writer.Key("autoHideCursorDelay");
	writer.Double(profile.autoHideCursorDelay);

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

	writer.Key("outputAlignment");
	writer.Uint((uint32_t)profile.outputAlignment);

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
		// 将任务栏图标替换为软件图标
		HINSTANCE hInst = wil::GetModuleInstanceHandle();
		ReplaceIcon(hInst, hWnd, true);
		ReplaceIcon(hInst, hWnd, false);

		// 删除标题栏中的图标
		INT_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
		SetWindowLongPtr(hWnd, GWL_STYLE, style & ~WS_SYSMENU);
	}

	return S_OK;
}

static void ShowErrorMessage(const wchar_t* mainInstruction, const wchar_t* content) noexcept {
	LocalizationService& ls = LocalizationService::Get();
	const winrt::hstring errorStr = ls.GetLocalizedString(L"AppSettings_Dialog_Error");
	const winrt::hstring exitStr = ls.GetLocalizedString(L"AppSettings_Dialog_Exit");

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

AppSettings& AppSettings::Get() noexcept {
	static AppSettings instance;
	return instance;
}

AppSettings::~AppSettings() {}

bool AppSettings::Initialize() noexcept {
	// 若程序所在目录存在配置文件则为便携模式
	_isPortableMode = Win32Helper::FileExists(StrHelper::Concat(
		CommonSharedConstants::CONFIG_DIR, L"\\", CommonSharedConstants::CONFIG_FILENAME).c_str());

	std::filesystem::path existingConfigPath;
	if (!_UpdateConfigPath(&existingConfigPath)) {
		Logger::Get().Error("_UpdateConfigPath 失败");
		return false;
	}

	Logger::Get().Info(StrHelper::Concat("便携模式: ", _isPortableMode ? "是" : "否"));

	if (existingConfigPath.empty()) {
		Logger::Get().Info("不存在配置文件");
		_SetDefaultScalingModes();
		_SetDefaultShortcuts();
		SaveAsync();
		return true;
	}

	// 此时 ResourceLoader 使用“首选语言”
	
	std::string configText;
	if (!Win32Helper::ReadTextFile(existingConfigPath.c_str(), configText)) {
		Logger::Get().Error("读取配置文件失败");

		LocalizationService& ls = LocalizationService::Get();
		winrt::hstring title = ls.GetLocalizedString(L"AppSettings_ErrorDialog_ReadFailed");
		winrt::hstring content = ls.GetLocalizedString(L"AppSettings_ErrorDialog_ConfigLocation");
		ShowErrorMessage(title.c_str(),
			fmt::format(fmt::runtime(std::wstring_view(content)), existingConfigPath.native()).c_str());
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

		LocalizationService& ls = LocalizationService::Get();
		winrt::hstring title = ls.GetLocalizedString(L"AppSettings_ErrorDialog_NotValidJson");
		winrt::hstring content = ls.GetLocalizedString(L"AppSettings_ErrorDialog_ConfigLocation");
		ShowErrorMessage(title.c_str(),
			fmt::format(fmt::runtime(std::wstring_view(content)), existingConfigPath.native()).c_str());
		return false;
	}

	if (!doc.IsObject()) {
		Logger::Get().Error("配置文件根元素不是 Object");
		LocalizationService& ls = LocalizationService::Get();
		winrt::hstring title = ls.GetLocalizedString(L"AppSettings_ErrorDialog_ParseFailed");
		winrt::hstring content = ls.GetLocalizedString(L"AppSettings_ErrorDialog_ConfigLocation");
		ShowErrorMessage(title.c_str(),
			fmt::format(fmt::runtime(std::wstring_view(content)), existingConfigPath.native()).c_str());
		return false;
	}

	_LoadSettings(((const rapidjson::Document&)doc).GetObj());

	// 迁移旧版配置后立刻保存，_SetDefaultShortcuts 用于确保快捷键不为空
	if (_SetDefaultShortcuts() || !Win32Helper::FileExists(_configPath.c_str())) {
		SaveAsync();
	}

	return true;
}

void AppSettings::Uninitialize() noexcept {
	// 等待后台保存完成
	_isSaving.wait(true, std::memory_order_relaxed);
}

// 确保写入失败时不会丢失旧配置
static bool SafeSaveConfig(const std::wstring& configPath, std::string_view json) noexcept {
	std::wstring newConfigPath = configPath + L".new";
	if (!Win32Helper::WriteTextFile(newConfigPath.c_str(), json)) {
		Logger::Get().Error("写入新配置文件失败");
		return false;
	}

	if (!DeleteFile(configPath.c_str())) {
		Logger::Get().Win32Error("DeleteFile 失败");
		return false;
	}

	if (!MoveFile(newConfigPath.c_str(), configPath.c_str())) {
		Logger::Get().Win32Error("MoveFile 失败");
		return false;
	}

	return true;
}

winrt::fire_and_forget AppSettings::SaveAsync() noexcept {
	_UpdateWindowPlacement();

	if (!Win32Helper::CreateDir(_configDir.native(), true)) {
		Logger::Get().Win32Error("创建配置文件夹失败");
		co_return;
	}

	rapidjson::StringBuffer json = _WriteConfigJson();

	// 等待前一次保存完成以确保配置文件始终是最新的。保存过于频繁时会阻塞主线程，
	// 但不会发生这种情况。
	_isSaving.wait(true, std::memory_order_relaxed);
	// 此时不存在竞争，无需 CAS 循环
	_isSaving.store(true, std::memory_order_relaxed);

	co_await winrt::resume_background();

	if (!SafeSaveConfig(_configPath.native(), { json.GetString(), json.GetLength() })) {
		Logger::Get().Error("保存配置文件失败");
	}
	
	_isSaving.store(false, std::memory_order_relaxed);
	// 只有主线程会等待
	_isSaving.notify_one();
}

void AppSettings::IsPortableMode(bool value) noexcept {
	if (_isPortableMode == value) {
		return;
	}

	if (!value) {
		// 关闭便携模式需删除本地配置文件
		if (!DeleteFile((_configDir / CommonSharedConstants::CONFIG_FILENAME).c_str())) {
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
	Logger::Get().Info(fmt::format("热键 {} 已更改为 {}", ShortcutHelper::ToString(action), value.ToString()));
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
	SaveAsync();

	// 更新启动任务
	if (AutoStartHelper::IsAutoStartEnabled()) {
		AutoStartHelper::EnableAutoStart(value);
	}
}

void AppSettings::IsShowNotifyIcon(bool value) noexcept {
	if (_isShowNotifyIcon == value) {
		return;
	}

	_isShowNotifyIcon = value;
	IsShowNotifyIconChanged.Invoke(value);

	SaveAsync();
}

static std::filesystem::path GetSystemScreenshotsDir() noexcept {
	// 如果 Screenshots 文件夹不存在将失败
	wil::unique_cotaskmem_string folder;
	HRESULT hr = SHGetKnownFolderPath(
		FOLDERID_Screenshots, KF_FLAG_DEFAULT, NULL, folder.put());
	if (SUCCEEDED(hr)) {
		return folder.get();
	}

	// 屏幕截图文件夹默认路径是 %USERPROFILE%\Pictures\Screenshots

	hr = SHGetKnownFolderPath(
		FOLDERID_Pictures, KF_FLAG_DEFAULT, NULL, folder.put());
	if (SUCCEEDED(hr)) {
		return StrHelper::Concat(folder.get(), L"\\Screenshots");
	}

	hr = SHGetKnownFolderPath(
		FOLDERID_Profile, KF_FLAG_DEFAULT, NULL, folder.put());
	if (SUCCEEDED(hr)) {
		return StrHelper::Concat(folder.get(), L"\\Pictures\\Screenshots");
	}
	
	Logger::Get().ComError("SHGetKnownFolderPath 失败", hr);
	return {};
}

static bool IsSubfolder(const std::wstring& sub, const std::wstring& parent) noexcept {
	if (!sub.starts_with(parent)) {
		return false;
	}

	if (parent.size() == sub.size()) {
		return true;
	}

	return sub[parent.size()] == L'\\';
}

// 失败时返回空字符串
std::filesystem::path AppSettings::ScreenshotsDir() const noexcept {
	if (_screenshotsDir.empty()) {
		// 系统“屏幕截图”文件夹
		return GetSystemScreenshotsDir();
	} else if (_screenshotsDir.is_relative()) {
		// 相对路径
		std::wstring workingDir;
		HRESULT hr = wil::GetCurrentDirectoryW(workingDir);
		if (FAILED(hr)) {
			Logger::Get().ComError("wil::GetCurrentDirectoryW 失败", hr);
			return {};
		}

		if (_screenshotsDir == L".") {
			return std::filesystem::path(std::move(workingDir));
		} else {
			return (std::filesystem::path(std::move(workingDir)) / _screenshotsDir).lexically_normal();
		}
	} else {
		// 绝对路径
		return _screenshotsDir;
	}
}

void AppSettings::ScreenshotsDir(const std::filesystem::path& value) noexcept {
	assert(!value.empty());

	if (value == GetSystemScreenshotsDir()) {
		// 系统“屏幕截图”文件夹
		_screenshotsDir.clear();
	} else {
		std::wstring workingDir;
		HRESULT hr = wil::GetCurrentDirectoryW(workingDir);
		if (FAILED(hr)) {
			Logger::Get().ComError("wil::GetCurrentDirectoryW 失败", hr);
			return;
		}

		if (IsSubfolder(value, workingDir)) {
			// 保存位置在工作文件夹内则转换为相对路径
			if (value.native().size() == workingDir.size()) {
				_screenshotsDir = L".";
			} else {
				_screenshotsDir = StrHelper::Concat(
					L".",
					std::wstring(value.native().begin() + workingDir.size(), value.native().end())
				);
			}
		} else {
			// 绝对路径
			_screenshotsDir = value;
		}
	}

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

	// rcNormalPosition 使用工作区坐标，应转换为屏幕坐标。
	// 见 https://github.com/Blinue/nt5src/blob/daad8a087a4e75422ec96b7911f1df4669989611/Source/XPSP1/NT/windows/core/ntuser/kernel/winmgr.c#L752
	HMONITOR hMon = MonitorFromWindow(hwndMain, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO mi{ sizeof(mi) };
	if (!GetMonitorInfo(hMon, &mi)) {
		Logger::Get().Win32Error("GetMonitorInfo 失败");
		return;
	}

	const LONG workingAreaOffsetX = mi.rcWork.left - mi.rcMonitor.left;
	const LONG workingAreaOffsetY = mi.rcWork.top - mi.rcMonitor.top;
	_mainWindowCenter = {
		(wp.rcNormalPosition.left + wp.rcNormalPosition.right) / 2.0f + workingAreaOffsetX,
		(wp.rcNormalPosition.top + wp.rcNormalPosition.bottom) / 2.0f + workingAreaOffsetY,
	};

	const float dpiFactor = GetDpiForWindow(hwndMain) / float(USER_DEFAULT_SCREEN_DPI);
	_mainWindowSizeInDips = {
		(wp.rcNormalPosition.right - wp.rcNormalPosition.left) / dpiFactor,
		(wp.rcNormalPosition.bottom - wp.rcNormalPosition.top) / dpiFactor,
	};

	_isMainWindowMaximized = wp.showCmd == SW_MAXIMIZE;
}

rapidjson::StringBuffer AppSettings::_WriteConfigJson() const noexcept {
	rapidjson::StringBuffer json;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(json);
	writer.StartObject();

	writer.Key("language");
	if (_language < 0) {
		writer.String("");
	} else {
		const wchar_t* language = LocalizationService::GetSupportedLanguages()[_language];
		writer.String(StrHelper::UTF16ToUTF8(language).c_str());
	}

	writer.Key("theme");
	writer.Uint((uint32_t)_theme);

	writer.Key("windowPos");
	writer.StartObject();
	writer.Key("centerX");
	writer.Double(_mainWindowCenter.X);
	writer.Key("centerY");
	writer.Double(_mainWindowCenter.Y);
	writer.Key("width");
	writer.Double(_mainWindowSizeInDips.Width);
	writer.Key("height");
	writer.Double(_mainWindowSizeInDips.Height);
	writer.Key("maximized");
	writer.Bool(_isMainWindowMaximized);
	writer.EndObject();

	writer.Key("shortcuts");
	writer.StartObject();
	writer.Key("scale");
	writer.Uint(EncodeShortcut(_shortcuts[(size_t)ShortcutAction::Scale]));
	writer.Key("windowedModeScale");
	writer.Uint(EncodeShortcut(_shortcuts[(size_t)ShortcutAction::WindowedModeScale]));
	writer.Key("toolbar");
	writer.Uint(EncodeShortcut(_shortcuts[(size_t)ShortcutAction::Toolbar]));
	writer.Key("takeScreenshot");
	writer.Uint(EncodeShortcut(_shortcuts[(size_t)ShortcutAction::TakeScreenshot]));
	writer.EndObject();

	writer.Key("countdownSeconds");
	writer.Uint(_countdownSeconds);
	writer.Key("developerMode");
	writer.Bool(_isDeveloperMode);
	writer.Key("debugMode");
	writer.Bool(_isDebugMode);
	writer.Key("benchmarkMode");
	writer.Bool(_isBenchmarkMode);
	writer.Key("disableTopmost");
	writer.Bool(_isTopmostDisabled);
	writer.Key("disableEffectCache");
	writer.Bool(_isEffectCacheDisabled);
	writer.Key("disableFontCache");
	writer.Bool(_isFontCacheDisabled);
	writer.Key("saveEffectSources");
	writer.Bool(_isSaveEffectSources);
	writer.Key("warningsAreErrors");
	writer.Bool(_isWarningsAreErrors);
	writer.Key("allowScalingMaximized");
	writer.Bool(_isAllowScalingMaximized);
	writer.Key("keepScreenOn");
	writer.Bool(_isKeepScreenOn);
	writer.Key("simulateExclusiveFullscreen");
	writer.Bool(_isSimulateExclusiveFullscreen);
	writer.Key("alwaysRunAsAdmin");
	writer.Bool(_isAlwaysRunAsAdmin);
	writer.Key("showNotifyIcon");
	writer.Bool(_isShowNotifyIcon);
	writer.Key("inlineParams");
	writer.Bool(_isInlineParams);
	writer.Key("autoCheckForUpdates");
	writer.Bool(_isAutoCheckForUpdates);
	writer.Key("checkForPreviewUpdates");
	writer.Bool(_isCheckForPreviewUpdates);
	writer.Key("updateCheckDate");
	writer.Int64(_updateCheckDate.time_since_epoch().count());
	writer.Key("duplicateFrameDetectionMode");
	writer.Uint((uint32_t)_duplicateFrameDetectionMode);
	writer.Key("enableStatisticsForDynamicDetection");
	writer.Bool(_isStatisticsForDynamicDetectionEnabled);
	writer.Key("minFrameRate");
	writer.Double(_minFrameRate);
	writer.Key("disableFP16");
	writer.Bool(_isFP16Disabled);

	ScalingModesService::Get().Export(writer);

	writer.Key("profiles");
	writer.StartArray();
	WriteProfile(writer, _defaultProfile);
	for (const Profile& rule : _profiles) {
		WriteProfile(writer, rule);
	}
	writer.EndArray();

	writer.Key("overlay");
	writer.StartObject();
	writer.Key("fullscreenInitialToolbarState");
	writer.Uint((uint32_t)_fullscreenInitialToolbarState);
	writer.Key("windowedInitialToolbarState");
	writer.Uint((uint32_t)_windowedInitialToolbarState);
	writer.Key("screenshotsDir");
	writer.String(StrHelper::UTF16ToUTF8(_screenshotsDir.native()).c_str());
	writer.Key("windows");
	writer.StartObject();
	for (const auto& [name, windowOption] : _overlayWindowOptions) {
		writer.Key(name.c_str());
		writer.StartObject();
		writer.Key("hArea");
		writer.Uint(windowOption.hArea);
		writer.Key("vArea");
		writer.Uint(windowOption.vArea);
		writer.Key("hPos");
		writer.Double(windowOption.hPos);
		writer.Key("vPos");
		writer.Double(windowOption.vPos);
		writer.EndObject();
	}
	writer.EndObject();
	writer.EndObject();

	writer.EndObject();

	return json;
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
			std::span<const wchar_t*> languages = LocalizationService::GetSupportedLanguages();
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
		auto windowPosObj = windowPosNode->value.GetObj();

		winrt::Point center{};
		winrt::Size size{};
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
					{ std::lround(_mainWindowCenter.X), std::lround(_mainWindowCenter.Y) },
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
		auto shortcutsObj = shortcutsNode->value.GetObj();

		auto scaleNode = shortcutsObj.FindMember("scale");
		if (scaleNode != shortcutsObj.MemberEnd() && scaleNode->value.IsUint()) {
			DecodeShortcut(scaleNode->value.GetUint(), _shortcuts[(size_t)ShortcutAction::Scale]);
		}

		auto windowedModeScaleNode = shortcutsObj.FindMember("windowedModeScale");
		if (windowedModeScaleNode != shortcutsObj.MemberEnd() && windowedModeScaleNode->value.IsUint()) {
			DecodeShortcut(windowedModeScaleNode->value.GetUint(), _shortcuts[(size_t)ShortcutAction::WindowedModeScale]);
		}

		auto toolbarNode = shortcutsObj.FindMember("toolbar");
		if (toolbarNode == shortcutsObj.MemberEnd()) {
			// v0.12 前使用 overlay
			toolbarNode = shortcutsObj.FindMember("overlay");
		}
		
		if (toolbarNode != shortcutsObj.MemberEnd() && toolbarNode->value.IsUint()) {
			DecodeShortcut(toolbarNode->value.GetUint(), _shortcuts[(size_t)ShortcutAction::Toolbar]);
		}

		auto takeScreenshotNode = shortcutsObj.FindMember("takeScreenshot");
		if (takeScreenshotNode != shortcutsObj.MemberEnd() && takeScreenshotNode->value.IsUint()) {
			DecodeShortcut(takeScreenshotNode->value.GetUint(), _shortcuts[(size_t)ShortcutAction::TakeScreenshot]);
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
	JsonHelper::ReadBool(root, "disableTopmost", _isTopmostDisabled);
	JsonHelper::ReadBool(root, "disableEffectCache", _isEffectCacheDisabled);
	JsonHelper::ReadBool(root, "disableFontCache", _isFontCacheDisabled);
	JsonHelper::ReadBool(root, "saveEffectSources", _isSaveEffectSources);
	JsonHelper::ReadBool(root, "warningsAreErrors", _isWarningsAreErrors);
	JsonHelper::ReadBool(root, "allowScalingMaximized", _isAllowScalingMaximized);
	JsonHelper::ReadBool(root, "keepScreenOn", _isKeepScreenOn);
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
	JsonHelper::ReadEnum(root, "duplicateFrameDetectionMode", _duplicateFrameDetectionMode);
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
		auto scaleProfilesArray = scaleProfilesNode->value.GetArray();

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

	auto overlayNode = root.FindMember("overlay");
	if (overlayNode != root.MemberEnd() && overlayNode->value.IsObject()) {
		auto overlayObj = overlayNode->value.GetObj();

		if (JsonHelper::ReadEnum(overlayObj, "fullscreenInitialToolbarState",
			_fullscreenInitialToolbarState, true)) {
			JsonHelper::ReadEnum(overlayObj, "windowedInitialToolbarState", _windowedInitialToolbarState);
		} else {
			// v0.12.0-preview1 中工具栏初始状态不区分全屏和窗口模式缩放
			JsonHelper::ReadEnum(overlayObj, "initialToolbarState", _fullscreenInitialToolbarState);
			_windowedInitialToolbarState = _fullscreenInitialToolbarState;
		}

		{
			std::wstring value;
			JsonHelper::ReadString(overlayObj, "screenshotsDir", value);
			_screenshotsDir = std::move(value);
		}

		auto windowsNode = overlayObj.FindMember("windows");
		if (windowsNode != overlayObj.MemberEnd() && windowsNode->value.IsObject()) {
			auto windowsObj = windowsNode->value.GetObj();

			const rapidjson::SizeType size = windowsObj.MemberCount();
			if (size > 0) {
				_overlayWindowOptions.reserve(size);

				for (const auto& windowOptionPair : windowsObj) {
					if (!windowOptionPair.value.IsObject()) {
						continue;
					}

					auto windowOptionObj = windowOptionPair.value.GetObj();

					OverlayWindowOption& windowOption = _overlayWindowOptions[windowOptionPair.name.GetString()];
					JsonHelper::ReadUInt16(windowOptionObj, "hArea", windowOption.hArea);
					JsonHelper::ReadUInt16(windowOptionObj, "vArea", windowOption.vArea);
					JsonHelper::ReadFloat(windowOptionObj, "hPos", windowOption.hPos);
					JsonHelper::ReadFloat(windowOptionObj, "vPos", windowOption.vPos);
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

		{
			std::wstring value;
			JsonHelper::ReadString(profileObj, "launcherPath", value);
			profile.launcherPath = std::move(value);
		}
		
		// 将旧版本的相对路径转换为绝对路径
		if (!profile.launcherPath.empty() && profile.launcherPath.is_relative()) {
			std::filesystem::path exePath(profile.pathRule);
			profile.launcherPath = (exePath.parent_path() / profile.launcherPath).lexically_normal();
		}

		{
			auto autoScaleNode = profileObj.FindMember("autoScale");
			if (autoScaleNode != profileObj.MemberEnd()) {
				if (autoScaleNode->value.IsUint()) {
					uint32_t value = autoScaleNode->value.GetUint();
					if (value >= (uint32_t)AutoScale::COUNT) {
						value = (uint32_t)AutoScale::Disabled;
					}
					profile.autoScale = (AutoScale)value;
				} else if (autoScaleNode->value.IsBool()) {
					// v0.12 前为布尔值
					profile.autoScale = autoScaleNode->value.GetBool() ?
						AutoScale::Fullscreen : AutoScale::Disabled;
				}
			}
		}
		
		JsonHelper::ReadString(profileObj, "launchParameters", profile.launchParameters);
	}

	JsonHelper::ReadInt(profileObj, "scalingMode", profile.scalingMode);
	if (profile.scalingMode < -1 || profile.scalingMode >= (int)_scalingModes.size()) {
		profile.scalingMode = -1;
	}

	if (!JsonHelper::ReadEnum(profileObj, "captureMethod", profile.captureMethod, true)) {
		// v0.10.0-preview1 使用 captureMode
		JsonHelper::ReadEnum(profileObj, "captureMode", profile.captureMethod);
	}

	// Desktop Duplication 捕获模式要求 Win10 20H1+
	if (profile.captureMethod == CaptureMethod::DesktopDuplication) {
		if (!Win32Helper::GetOSVersion().Is20H1OrNewer()) {
			profile.captureMethod = CaptureMethod::GraphicsCapture;
		}
	}

	JsonHelper::ReadEnum(profileObj, "multiMonitorUsage", profile.multiMonitorUsage);
	JsonHelper::ReadEnum(profileObj, "initialWindowedScaleFactor", profile.initialWindowedScaleFactor);

	JsonHelper::ReadFloat(profileObj, "customInitialWindowedScaleFactor",
		profile.customInitialWindowedScaleFactor);
	if (profile.customInitialWindowedScaleFactor < 1.0f) {
		profile.customInitialWindowedScaleFactor = 1.0f;
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
	if (profile.maxFrameRate <= 10.0f - FLOAT_EPSILON<float> ||
		profile.maxFrameRate >= 1000.0f + FLOAT_EPSILON<float>)
	{
		profile.maxFrameRate = 60.0f;
	}

	JsonHelper::ReadBoolFlag(profileObj, "3DGameMode", ScalingFlags::Is3DGameMode, profile.scalingFlags);
	if (!JsonHelper::ReadBoolFlag(profileObj, "captureTitleBar", ScalingFlags::CaptureTitleBar, profile.scalingFlags, true)) {
		// v0.10.0-preview1 使用 reserveTitleBar
		JsonHelper::ReadBoolFlag(profileObj, "reserveTitleBar", ScalingFlags::CaptureTitleBar, profile.scalingFlags);
	}
	JsonHelper::ReadBoolFlag(profileObj, "adjustCursorSpeed", ScalingFlags::AdjustCursorSpeed, profile.scalingFlags);
	JsonHelper::ReadBoolFlag(profileObj, "disableDirectFlip", ScalingFlags::DisableDirectFlip, profile.scalingFlags);

	JsonHelper::ReadEnum(profileObj, "cursorScaling", profile.cursorScaling);
	
	JsonHelper::ReadFloat(profileObj, "customCursorScaling", profile.customCursorScaleFactor);
	if (profile.customCursorScaleFactor < 0) {
		profile.customCursorScaleFactor = 1.0f;
	}

	JsonHelper::ReadEnum(profileObj, "cursorInterpolationMode", profile.cursorInterpolationMode);
	JsonHelper::ReadBool(profileObj, "autoHideCursorEnabled", profile.isAutoHideCursorEnabled);
	JsonHelper::ReadFloat(profileObj, "autoHideCursorDelay", profile.autoHideCursorDelay);
	if (profile.autoHideCursorDelay <= 0.1f - FLOAT_EPSILON<float> ||
		profile.autoHideCursorDelay >= 5.0f + FLOAT_EPSILON<float>)
	{
		profile.autoHideCursorDelay = 3.0f;
	}

	JsonHelper::ReadBool(profileObj, "croppingEnabled", profile.isCroppingEnabled);

	auto croppingNode = profileObj.FindMember("cropping");
	if (croppingNode != profileObj.MemberEnd() && croppingNode->value.IsObject()) {
		auto croppingObj = croppingNode->value.GetObj();

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

	JsonHelper::ReadEnum(profileObj, "outputAlignment", profile.outputAlignment);

	return true;
}

bool AppSettings::_SetDefaultShortcuts() noexcept {
	bool changed = false;

	Shortcut& scaleShortcut = _shortcuts[(size_t)ShortcutAction::Scale];
	if (scaleShortcut.IsEmpty()) {
		scaleShortcut.alt = true;
		scaleShortcut.shift = true;
		scaleShortcut.code = 'A';

		changed = true;
	}

	Shortcut& windowedModeScaleShortcut = _shortcuts[(size_t)ShortcutAction::WindowedModeScale];
	if (windowedModeScaleShortcut.IsEmpty()) {
		windowedModeScaleShortcut.alt = true;
		windowedModeScaleShortcut.shift = true;
		windowedModeScaleShortcut.code = 'Q';

		changed = true;
	}

	Shortcut& overlayShortcut = _shortcuts[(size_t)ShortcutAction::Toolbar];
	if (overlayShortcut.IsEmpty()) {
		overlayShortcut.alt = true;
		overlayShortcut.shift = true;
		overlayShortcut.code = 'D';

		changed = true;
	}

	Shortcut& takeScreenshotShortcut = _shortcuts[(size_t)ShortcutAction::TakeScreenshot];
	if (takeScreenshotShortcut.IsEmpty()) {
		takeScreenshotShortcut.alt = true;
		takeScreenshotShortcut.shift = true;
		takeScreenshotShortcut.code = 'S';

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
	// CuNNy
	{
		auto& acnet = _scalingModes[3];
		acnet.name = L"CuNNy";
		acnet.effects.emplace_back().name = L"CuNNy2\\CuNNy-4x12-NVL";
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
			L"{}\\Magpie\\{}\\v{}\\{}",
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
		L"\\",
		CommonSharedConstants::CONFIG_FILENAME
	);

	if (Win32Helper::FileExists(v1ConfigPath.c_str())) {
		return v1ConfigPath;
	}

	return {};
}

bool AppSettings::_UpdateConfigPath(std::filesystem::path* existingConfigPath) noexcept {
	if (_isPortableMode) {
		std::wstring value;
		HRESULT hr = wil::GetFullPathNameW(CommonSharedConstants::CONFIG_DIR, value);
		if (FAILED(hr)) {
			Logger::Get().ComError("GetFullPathNameW 失败", hr);
			return false;
		}
		_configDir = std::move(value);

		_configPath = _configDir / CommonSharedConstants::CONFIG_FILENAME;

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

		_configDir = fmt::format(L"{}\\Magpie\\{}\\v{}\\",
			localAppDataDir.get(), CommonSharedConstants::CONFIG_DIR, CONFIG_VERSION);
		_configPath = _configDir / CommonSharedConstants::CONFIG_FILENAME;

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
	if (!Win32Helper::CreateDir(_configDir.native(), true)) {
		Logger::Get().Win32Error("创建配置文件夹失败");
		return false;
	}

	return true;
}

}
