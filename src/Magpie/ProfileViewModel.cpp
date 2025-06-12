#include "pch.h"
#include "ProfileViewModel.h"
#if __has_include("ProfileViewModel.g.cpp")
#include "ProfileViewModel.g.cpp"
#endif
#include "Profile.h"
#include "AppXReader.h"
#include "IconHelper.h"
#include "ProfileService.h"
#include "StrHelper.h"
#include "Win32Helper.h"
#include "AppSettings.h"
#include "Logger.h"
#include "ScalingMode.h"
#include "ScalingService.h"
#include "FileDialogHelper.h"
#include "CommonSharedConstants.h"
#include "App.h"
#include "MainWindow.h"
#include "AdaptersService.h"

using namespace ::Magpie;
using namespace winrt;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media::Imaging;

namespace winrt::Magpie::implementation {

ProfileViewModel::ProfileViewModel(int profileIdx) : _isDefaultProfile(profileIdx < 0) {
	if (_isDefaultProfile) {
		_data = &ProfileService::Get().DefaultProfile();
	} else {
		_index = (uint32_t)profileIdx;
		_data = &ProfileService::Get().GetProfile(profileIdx);

		// 占位
		_icon = FontIcon();

		_appThemeChangedRevoker = App::Get().ThemeChanged(auto_revoke, [this](bool) { _LoadIcon(); });
		_dpiChangedRevoker = App::Get().MainWindow().DpiChanged(
			auto_revoke, [this](uint32_t) { _LoadIcon(); });

		if (_data->isPackaged) {
			AppXReader appxReader;
			_isProgramExist = appxReader.Initialize(_data->pathRule);
		} else {
			_isProgramExist = Win32Helper::FileExists(_data->pathRule.c_str());
		}

		_LoadIcon();
	}

	_adaptersChangedRevoker = AdaptersService::Get().AdaptersChanged(auto_revoke,
		std::bind_front(&ProfileViewModel::_AdaptersService_AdaptersChanged, this));
}

ProfileViewModel::~ProfileViewModel() {}

bool ProfileViewModel::IsNotDefaultProfile() const noexcept {
	return !_data->name.empty();
}

bool ProfileViewModel::IsNotPackaged() const noexcept {
	return !_data->isPackaged;
}

fire_and_forget ProfileViewModel::OpenProgramLocation() const noexcept {
	if (!_isProgramExist) {
		co_return;
	}

	std::wstring programLocation;
	if (_data->isPackaged) {
		AppXReader appxReader;
		[[maybe_unused]] bool result = appxReader.Initialize(_data->pathRule);
		assert(result);

		programLocation = appxReader.GetExecutablePath();
		if (programLocation.empty()) {
			// 找不到可执行文件则打开应用文件夹
			Win32Helper::ShellOpen(appxReader.GetPackagePath().c_str());
			co_return;
		}
	} else {
		programLocation = _data->pathRule;
	}

	co_await resume_background();
	Win32Helper::OpenFolderAndSelectFile(programLocation.c_str());
}

static std::wstring ExtractFolder(const std::wstring& path) noexcept {
	const size_t delimPos = path.find_last_of(L'\\');
	if (delimPos == std::wstring::npos) {
		assert(false);
		return {};
	}

	std::wstring result = path.substr(0, delimPos);
	if (Win32Helper::DirExists(result.c_str())) {
		return result;
	} else {
		return {};
	}
}

static std::wstring GetStartFolderForSettingLauncher(const Profile& profile) noexcept {
	if (profile.launcherPath.empty()) {
		// 无启动器
		return ExtractFolder(profile.pathRule);
	}

	std::wstring result = ExtractFolder(profile.launcherPath);
	if (result.empty()) {
		// 启动器路径失效
		return ExtractFolder(profile.pathRule);
	} else {
		return result;
	}
}

void ProfileViewModel::ChangeExeForLaunching() const noexcept {
	if (!_isProgramExist || _data->isPackaged) {
		return;
	}

	com_ptr<IFileOpenDialog> fileDialog = try_create_instance<IFileOpenDialog>(CLSID_FileOpenDialog);
	if (!fileDialog) {
		Logger::Get().Error("创建 FileSaveDialog 失败");
		return;
	}

	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	static std::wstring titleStr(resourceLoader.GetString(L"Dialog_SelectLauncher_Title"));
	fileDialog->SetTitle(titleStr.c_str());

	static std::wstring exeFileStr(resourceLoader.GetString(L"Dialog_ExeFile"));
	const COMDLG_FILTERSPEC fileType{ exeFileStr.c_str(), L"*.exe"};
	fileDialog->SetFileTypes(1, &fileType);
	fileDialog->SetDefaultExtension(L"exe");

	std::wstring startFolder = GetStartFolderForSettingLauncher(*_data);
	if (!startFolder.empty()) {
		com_ptr<IShellItem> shellItem;
		SHCreateItemFromParsingName(startFolder.c_str(), nullptr, IID_PPV_ARGS(&shellItem));
		fileDialog->SetFolder(shellItem.get());
	}

	// GH#1158
	// 选择 exe 的快捷方式时 IFileOpenDialog 默认解析它指向的文件路径，导致参数丢失。
	// FOS_NODEREFERENCELINKS 可以禁止解析，直接返回 lnk 文件。
	std::optional<std::filesystem::path> launcherPath = FileDialogHelper::OpenFileDialog(
		fileDialog.get(), FOS_STRICTFILETYPES | FOS_NODEREFERENCELINKS);
	if (!launcherPath || launcherPath->empty() || *launcherPath == _data->pathRule) {
		return;
	}

	_data->launcherPath = std::move(*launcherPath);
	AppSettings::Get().SaveAsync();
}

hstring ProfileViewModel::Name() const noexcept {
	if (_data->name.empty()) {
		return ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID)
			.GetString(L"Root_Defaults/Content");
	} else {
		return hstring(_data->name);
	}
}

static void LaunchPackagedApp(const Profile& profile) noexcept {
	// 关于启动打包应用的讨论:
	// https://github.com/microsoft/WindowsAppSDK/issues/2856#issuecomment-1224409948
	// 使用 CLSCTX_LOCAL_SERVER 以在独立的进程中启动应用
	// 见 https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-iapplicationactivationmanager
	com_ptr<IApplicationActivationManager> aam =
		try_create_instance<IApplicationActivationManager>(CLSID_ApplicationActivationManager, CLSCTX_LOCAL_SERVER);
	if (!aam) {
		Logger::Get().Error("创建 ApplicationActivationManager 失败");
		return;
	}

	// 确保启动为前台窗口
	HRESULT hr = CoAllowSetForegroundWindow(aam.get(), nullptr);
	if (FAILED(hr)) {
		Logger::Get().ComError("创建 CoAllowSetForegroundWindow 失败", hr);
	}

	DWORD procId;
	hr = aam->ActivateApplication(profile.pathRule.c_str(), profile.launchParameters.c_str(), AO_NONE, &procId);
	if (FAILED(hr)) {
		Logger::Get().ComError("IApplicationActivationManager::ActivateApplication 失败", hr);
		return;
	}
}

static void LaunchWin32App(const Profile& profile) noexcept {
	const std::wstring& path = !profile.launcherPath.empty() &&
		Win32Helper::FileExists(profile.launcherPath.c_str()) ? profile.launcherPath.native() : profile.pathRule;
	Win32Helper::ShellOpen(path.c_str(), profile.launchParameters.c_str());
}

void ProfileViewModel::Launch() const noexcept {
	if (!_isProgramExist) {
		return;
	}

	if (_data->isPackaged) {
		LaunchPackagedApp(*_data);
	} else {
		LaunchWin32App(*_data);
	}
}

void ProfileViewModel::RenameText(const hstring& value) {
	_renameText = value;
	RaisePropertyChanged(L"RenameText");

	_trimedRenameText = value;
	StrHelper::Trim(_trimedRenameText);
	bool newEnabled = !_trimedRenameText.empty() && _trimedRenameText != _data->name;
	if (_isRenameConfirmButtonEnabled != newEnabled) {
		_isRenameConfirmButtonEnabled = newEnabled;
		RaisePropertyChanged(L"IsRenameConfirmButtonEnabled");
	}
}

void ProfileViewModel::Rename() {
	if (_isDefaultProfile || !_isRenameConfirmButtonEnabled) {
		return;
	}

	ProfileService::Get().RenameProfile(_index, _trimedRenameText);
	RaisePropertyChanged(L"Name");
}

bool ProfileViewModel::CanMoveUp() const noexcept {
	return !_isDefaultProfile && _index != 0;
}

bool ProfileViewModel::CanMoveDown() const noexcept {
	return !_isDefaultProfile && _index + 1 < ProfileService::Get().GetProfileCount();
}

void ProfileViewModel::MoveUp() {
	if (_isDefaultProfile) {
		return;
	}

	ProfileService& profileService = ProfileService::Get();
	if (!profileService.MoveProfile(_index, true)) {
		return;
	}

	--_index;
	_data = &profileService.GetProfile(_index);

	RaisePropertyChanged(L"CanMoveUp");
	RaisePropertyChanged(L"CanMoveDown");
}

void ProfileViewModel::MoveDown() {
	if (_isDefaultProfile) {
		return;
	}

	ProfileService& profileService = ProfileService::Get();
	if (!profileService.MoveProfile(_index, false)) {
		return;
	}

	++_index;
	_data = &profileService.GetProfile(_index);

	RaisePropertyChanged(L"CanMoveUp");
	RaisePropertyChanged(L"CanMoveDown");
}

void ProfileViewModel::Delete() {
	if (_isDefaultProfile) {
		return;
	}

	ProfileService::Get().RemoveProfile(_index);
	_data = nullptr;
}

IVector<IInspectable> ProfileViewModel::ScalingModes() const noexcept {
	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	
	std::vector<IInspectable> scalingModes;
	scalingModes.push_back(box_value(resourceLoader.GetString(L"Profile_General_ScalingMode_None")));
	for (const ::Magpie::ScalingMode& sm : AppSettings::Get().ScalingModes()) {
		scalingModes.push_back(box_value(sm.name));
	}

	return single_threaded_vector(std::move(scalingModes));
}

int ProfileViewModel::ScalingMode() const noexcept {
	return _data->scalingMode + 1;
}

void ProfileViewModel::ScalingMode(int value) {
	_data->scalingMode = value - 1;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"ScalingMode");
}

IVector<IInspectable> ProfileViewModel::CaptureMethods() const noexcept {
	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);

	std::vector<IInspectable> captureMethods;
	captureMethods.reserve(4);
	captureMethods.push_back(box_value(L"Graphics Capture"));
	if (Win32Helper::GetOSVersion().Is20H1OrNewer()) {
		// Desktop Duplication 要求 Win10 20H1+
		captureMethods.push_back(box_value(L"Desktop Duplication"));
	}
	captureMethods.push_back(box_value(L"GDI"));
	captureMethods.push_back(box_value(L"DwmSharedSurface"));

	return single_threaded_vector(std::move(captureMethods));
}

int ProfileViewModel::CaptureMethod() const noexcept {
	if (Win32Helper::GetOSVersion().Is20H1OrNewer() || _data->captureMethod < CaptureMethod::DesktopDuplication) {
		return (int)_data->captureMethod;
	} else {
		return (int)_data->captureMethod - 1;
	}
}

void ProfileViewModel::CaptureMethod(int value) {
	if (value < 0) {
		return;
	}

	if (!Win32Helper::GetOSVersion().Is20H1OrNewer() && value >= (int)CaptureMethod::DesktopDuplication) {
		++value;
	}

	enum CaptureMethod captureMethod = (enum CaptureMethod)value;
	if (_data->captureMethod == captureMethod) {
		return;
	}

	_data->captureMethod = captureMethod;
	RaisePropertyChanged(L"CaptureMethod");
	RaisePropertyChanged(L"CanCaptureTitleBar");

	AppSettings::Get().SaveAsync();
}

bool ProfileViewModel::IsAutoScale() const noexcept {
	return _data->isAutoScale;
}

void ProfileViewModel::IsAutoScale(bool value) {
	if (_data->isAutoScale == value) {
		return;
	}

	_data->isAutoScale = value;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"IsAutoScale");

	if (value) {
		// 立即检查前台窗口是否应自动缩放
		ScalingService::Get().CheckForeground();
	}
}

bool ProfileViewModel::Is3DGameMode() const noexcept {
	return _data->Is3DGameMode();
}

void ProfileViewModel::Is3DGameMode(bool value) {
	if (_data->Is3DGameMode() == value) {
		return;
	}

	_data->Is3DGameMode(value);
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"Is3DGameMode");
}

bool ProfileViewModel::HasMultipleMonitors() const noexcept {
	return GetSystemMetrics(SM_CMONITORS) > 1;
}

int ProfileViewModel::MultiMonitorUsage() const noexcept {
	return (int)_data->multiMonitorUsage;
}

void ProfileViewModel::MultiMonitorUsage(int value) {
	if (value < 0) {
		return;
	}

	::Magpie::MultiMonitorUsage multiMonitorUsage = (::Magpie::MultiMonitorUsage)value;
	if (_data->multiMonitorUsage == multiMonitorUsage) {
		return;
	}

	_data->multiMonitorUsage = multiMonitorUsage;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"MultiMonitorUsage");
}

int ProfileViewModel::InitialWindowedScaleFactor() const noexcept {
	return (int)_data->initialWindowedScaleFactor;
}

void ProfileViewModel::InitialWindowedScaleFactor(int value) {
	if (value < 0) {
		return;
	}

	::Magpie::InitialWindowedScaleFactor factor = (::Magpie::InitialWindowedScaleFactor)value;
	if (_data->initialWindowedScaleFactor == factor) {
		return;
	}

	_data->initialWindowedScaleFactor = factor;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"InitialWindowedScaleFactor");
}

double ProfileViewModel::CustomInitialWindowedScaleFactor() const noexcept {
	return _data->customInitialWindowedScaleFactor;
}

void ProfileViewModel::CustomInitialWindowedScaleFactor(double value) {
	if (_data->customInitialWindowedScaleFactor == value) {
		return;
	}

	_data->customInitialWindowedScaleFactor = std::isnan(value) ? 1.0f : (float)value;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"CustomInitialWindowedScaleFactor");
}

IVector<IInspectable> ProfileViewModel::GraphicsCards() const noexcept {
	std::vector<IInspectable> graphicsCards;

	if (IsShowGraphicsCardSettingsCard()) {
		const std::vector<AdapterInfo>& adapterInfos = AdaptersService::Get().AdapterInfos();
		graphicsCards.reserve(adapterInfos.size() + 1);

		ResourceLoader resourceLoader =
			ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
		hstring defaultStr = resourceLoader.GetString(L"Profile_General_CaptureMethod_Default");

		// “默认”选项中显示实际使用的显卡
		graphicsCards.push_back(box_value(
			StrHelper::Concat(defaultStr, L" (", adapterInfos[0].description, L")")));

		for (const AdapterInfo& adapterInfo : adapterInfos) {
			graphicsCards.push_back(box_value(adapterInfo.description));
		}
	}

	return single_threaded_vector(std::move(graphicsCards));
}

int ProfileViewModel::GraphicsCard() const noexcept {
	if (_data->graphicsCardId.idx < 0) {
		return 0;
	}

	// 不要把 graphicsCardId.idx 当作位序使用，AdaptersService 会过滤掉不支持 FL11 的显卡
	const std::vector<AdapterInfo>& adapterInfos = AdaptersService::Get().AdapterInfos();
	auto it = std::find_if(adapterInfos.begin(), adapterInfos.end(),
		[&](const AdapterInfo& ai) { return (int)ai.idx == _data->graphicsCardId.idx; });
	if (it == adapterInfos.end()) {
		assert(false);
		return 0;
	}

	return int(it - adapterInfos.begin()) + 1;
}

void ProfileViewModel::GraphicsCard(int value) {
	if (value < 0 || _isHandlingAdapterChanged) {
		return;
	}

	GraphicsCardId& gcid = _data->graphicsCardId;

	const std::vector<AdapterInfo>& adapterInfos = AdaptersService::Get().AdapterInfos();
	if (value == 0 || value - 1 >= (int)adapterInfos.size()) {
		// 设为默认显卡，并清空 vendorId 和 deviceId
		gcid.idx = -1;
		gcid.vendorId = 0;
		gcid.deviceId = 0;
	} else {
		const AdapterInfo& ai = adapterInfos[size_t(value - 1)];
		gcid.idx = ai.idx;
		gcid.vendorId = ai.vendorId;
		gcid.deviceId = ai.deviceId;
	}
	
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"GraphicsCard");
}

bool ProfileViewModel::IsShowGraphicsCardSettingsCard() const noexcept {
	return AdaptersService::Get().AdapterInfos().size() > 1;
}

bool ProfileViewModel::IsNoGraphicsCard() const noexcept {
	return AdaptersService::Get().AdapterInfos().empty();
}

bool ProfileViewModel::IsFrameRateLimiterEnabled() const noexcept {
	return _data->isFrameRateLimiterEnabled;
}

void ProfileViewModel::IsFrameRateLimiterEnabled(bool value) {
	if (_data->isFrameRateLimiterEnabled == value) {
		return;
	}

	_data->isFrameRateLimiterEnabled = value;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"IsFrameRateLimiterEnabled");
}

double ProfileViewModel::MaxFrameRate() const noexcept {
	return _data->maxFrameRate;
}

void ProfileViewModel::MaxFrameRate(double value) {
	if (_data->maxFrameRate == value) {
		return;
	}

	// 用户已清空数字框则重置为 60
	_data->maxFrameRate = std::isnan(value) ? 60.0f : (float)value;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"MaxFrameRate");
}

bool ProfileViewModel::IsCaptureTitleBar() const noexcept {
	return _data->IsCaptureTitleBar();
}

void ProfileViewModel::IsCaptureTitleBar(bool value) {
	if (_data->IsCaptureTitleBar() == value) {
		return;
	}

	_data->IsCaptureTitleBar(value);
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"IsCaptureTitleBar");
}

bool ProfileViewModel::CanCaptureTitleBar() const noexcept {
	return _data->captureMethod == CaptureMethod::GraphicsCapture
		|| _data->captureMethod == CaptureMethod::DesktopDuplication;
}

bool ProfileViewModel::IsCroppingEnabled() const noexcept {
	return _data->isCroppingEnabled;
}

void ProfileViewModel::IsCroppingEnabled(bool value) {
	if (_data->isCroppingEnabled == value) {
		return;
	}

	_data->isCroppingEnabled = value;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"IsCroppingEnabled");
}

double ProfileViewModel::CroppingLeft() const noexcept {
	return _data->cropping.Left;
}

void ProfileViewModel::CroppingLeft(double value) {
	if (_data->cropping.Left == value) {
		return;
	}

	_data->cropping.Left = std::isnan(value) ? 0.0f : (float)value;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"CroppingLeft");
}

double ProfileViewModel::CroppingTop() const noexcept {
	return _data->cropping.Top;
}

void ProfileViewModel::CroppingTop(double value) {
	if (_data->cropping.Top == value) {
		return;
	}

	// 用户已清空数字框则重置为 0
	_data->cropping.Top = std::isnan(value) ? 0.0f : (float)value;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"CroppingTop");
}

double ProfileViewModel::CroppingRight() const noexcept {
	return _data->cropping.Right;
}

void ProfileViewModel::CroppingRight(double value) {
	if (_data->cropping.Right == value) {
		return;
	}

	_data->cropping.Right = std::isnan(value) ? 0.0f : (float)value;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"CroppingRight");
}

double ProfileViewModel::CroppingBottom() const noexcept {
	return _data->cropping.Bottom;
}

void ProfileViewModel::CroppingBottom(double value) {
	if (_data->cropping.Bottom == value) {
		return;
	}

	_data->cropping.Bottom = std::isnan(value) ? 0.0f : (float)value;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"CroppingBottom");
}

bool ProfileViewModel::IsAdjustCursorSpeed() const noexcept {
	return _data->IsAdjustCursorSpeed();
}

void ProfileViewModel::IsAdjustCursorSpeed(bool value) {
	if (_data->IsAdjustCursorSpeed() == value) {
		return;
	}

	_data->IsAdjustCursorSpeed(value);
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"IsAdjustCursorSpeed");
}

int ProfileViewModel::CursorScaling() const noexcept {
	return (int)_data->cursorScaling;
}

void ProfileViewModel::CursorScaling(int value) {
	if (value < 0) {
		return;
	}

	::Magpie::CursorScaling cursorScaling = (::Magpie::CursorScaling)value;
	if (_data->cursorScaling == cursorScaling) {
		return;
	}

	_data->cursorScaling = cursorScaling;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"CursorScaling");
}

double ProfileViewModel::CustomCursorScaling() const noexcept {
	return _data->customCursorScaling;
}

void ProfileViewModel::CustomCursorScaling(double value) {
	if (_data->customCursorScaling == value) {
		return;
	}

	_data->customCursorScaling = std::isnan(value) ? 1.0f : (float)value;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"CustomCursorScaling");
}

int ProfileViewModel::CursorInterpolationMode() const noexcept {
	return (int)_data->cursorInterpolationMode;
}

void ProfileViewModel::CursorInterpolationMode(int value) {
	if (value < 0) {
		return;
	}

	::Magpie::CursorInterpolationMode cursorInterpolationMode = (::Magpie::CursorInterpolationMode)value;
	if (_data->cursorInterpolationMode == cursorInterpolationMode) {
		return;
	}

	_data->cursorInterpolationMode = cursorInterpolationMode;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"CursorInterpolationMode");
}

hstring ProfileViewModel::LaunchParameters() const noexcept {
	return hstring(_data->launchParameters);
}

void ProfileViewModel::LaunchParameters(const hstring& value) {
	std::wstring_view trimmed(value);
	StrHelper::Trim(trimmed);
	_data->launchParameters = trimmed;
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"LaunchParameters");
}

bool ProfileViewModel::IsDirectFlipDisabled() const noexcept {
	return _data->IsDirectFlipDisabled();
}

void ProfileViewModel::IsDirectFlipDisabled(bool value) {
	if (_data->IsDirectFlipDisabled() == value) {
		return;
	}

	_data->IsDirectFlipDisabled(value);
	AppSettings::Get().SaveAsync();

	RaisePropertyChanged(L"IsDirectFlipDisabled");
}

fire_and_forget ProfileViewModel::_LoadIcon() {
	std::wstring iconPath;
	SoftwareBitmap iconBitmap{ nullptr };

	auto weakThis = get_weak();

	if (_isProgramExist) {
		const bool preferLightTheme = App::Get().IsLightTheme();
		const bool isPackaged = _data->isPackaged;
		const std::wstring path = _data->pathRule;
		const uint32_t iconSize =
			(uint32_t)std::lroundf(32.0f * App::Get().MainWindow().CurrentDpi() / USER_DEFAULT_SCREEN_DPI);

		co_await resume_background();

		if (isPackaged) {
			AppXReader appxReader;
			[[maybe_unused]] bool result = appxReader.Initialize(path);
			assert(result);

			std::variant<std::wstring, SoftwareBitmap> uwpIcon =
				appxReader.GetIcon(iconSize, preferLightTheme);
			if (uwpIcon.index() == 0) {
				iconPath = std::get<0>(uwpIcon);
			} else {
				iconBitmap = std::get<1>(uwpIcon);
			}
		} else {
			iconBitmap = IconHelper::ExtractIconFromExe(path.c_str(), iconSize);
		}

		co_await App::Get().Dispatcher();
		if (!weakThis.get()) {
			co_return;
		}
	}

	if (!iconPath.empty()) {
		BitmapIcon icon;
		icon.ShowAsMonochrome(false);
		icon.UriSource(Uri(iconPath));

		_icon = std::move(icon);
	} else if (iconBitmap) {
		SoftwareBitmapSource imageSource;
		co_await imageSource.SetBitmapAsync(iconBitmap);

		if (!weakThis.get()) {
			co_return;
		}

		MUXC::ImageIcon imageIcon;
		imageIcon.Source(imageSource);

		_icon = std::move(imageIcon);
	} else {
		_icon = nullptr;
	}

	RaisePropertyChanged(L"Icon");
}

void ProfileViewModel::_AdaptersService_AdaptersChanged() {
	_isHandlingAdapterChanged = true;
	RaisePropertyChanged(L"IsShowGraphicsCardSettingsCard");
	RaisePropertyChanged(L"IsNoGraphicsCard");
	RaisePropertyChanged(L"GraphicsCards");
	RaisePropertyChanged(L"GraphicsCard");
	_isHandlingAdapterChanged = false;
}

}
