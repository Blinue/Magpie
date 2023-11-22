#include "pch.h"
#include "ProfileViewModel.h"
#if __has_include("ProfileViewModel.g.cpp")
#include "ProfileViewModel.g.cpp"
#endif
#include "Profile.h"
#include "AppXReader.h"
#include "IconHelper.h"
#include "ProfileService.h"
#include "StrUtils.h"
#include <Magpie.Core.h>
#include "Win32Utils.h"
#include "AppSettings.h"
#include "Logger.h"
#include "ScalingMode.h"
#include <dxgi.h>
#include "MagService.h"
#include "FileDialogHelper.h"

using namespace winrt;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace ::Magpie::Core;

namespace winrt::Magpie::App::implementation {

static SmallVector<std::wstring> GetAllGraphicsCards() {
	com_ptr<IDXGIFactory1> dxgiFactory;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		return {};
	}

	SmallVector<std::wstring> result;

	com_ptr<IDXGIAdapter1> adapter;
	for (UINT adapterIndex = 0;
		SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, adapter.put()));
		++adapterIndex
		) {
		DXGI_ADAPTER_DESC1 desc;
		hr = adapter->GetDesc1(&desc);

		// 不包含 WARP
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}

		result.emplace_back(SUCCEEDED(hr) ? desc.Description : L"???");
	}

	return result;
}

ProfileViewModel::ProfileViewModel(int profileIdx) : _isDefaultProfile(profileIdx < 0) {
	if (_isDefaultProfile) {
		_data = &ProfileService::Get().DefaultProfile();
	} else {
		_index = (uint32_t)profileIdx;
		_data = &ProfileService::Get().GetProfile(profileIdx);

		// 占位
		_icon = FontIcon();

		App app = Application::Current().as<App>();
		RootPage rootPage = app.RootPage();
		_themeChangedRevoker = rootPage.ActualThemeChanged(
			auto_revoke,
			[this](FrameworkElement const& sender, IInspectable const&) {
				_LoadIcon(sender);
			}
		);

		_displayInformation = DisplayInformation::GetForCurrentView();
		_dpiChangedRevoker = _displayInformation.DpiChanged(
			auto_revoke,
			[this](DisplayInformation const&, IInspectable const&) {
				if (RootPage rootPage = Application::Current().as<App>().RootPage()) {
					_LoadIcon(rootPage);
				}
			}
		);

		if (_data->isPackaged) {
			AppXReader appxReader;
			_isProgramExist = appxReader.Initialize(_data->pathRule);
		} else {
			_isProgramExist = Win32Utils::FileExists(_data->pathRule.c_str());
		}

		_LoadIcon(rootPage);
	}

	ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView();
	{
		std::vector<IInspectable> scalingModes;
		scalingModes.push_back(box_value(resourceLoader.GetString(L"Profile_General_ScalingMode_None")));
		for (const Magpie::App::ScalingMode& sm : AppSettings::Get().ScalingModes()) {
			scalingModes.push_back(box_value(sm.name));
		}
		_scalingModes = single_threaded_vector(std::move(scalingModes));
	}

	{
		std::vector<IInspectable> captureMethods;
		captureMethods.reserve(4);
		captureMethods.push_back(box_value(L"Graphics Capture"));
		if (Win32Utils::GetOSVersion().Is20H1OrNewer()) {
			// Desktop Duplication 要求 Win10 20H1+
			captureMethods.push_back(box_value(L"Desktop Duplication"));
		}
		captureMethods.push_back(box_value(L"GDI"));
		captureMethods.push_back(box_value(L"DwmSharedSurface"));

		_captureMethods = single_threaded_vector(std::move(captureMethods));
	}

	_graphicsCards = GetAllGraphicsCards();
	if (_data->graphicsCard >= _graphicsCards.size()) {
		_data->graphicsCard = -1;
	}
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
			Win32Utils::ShellOpen(appxReader.GetPackagePath().c_str());
			co_return;
		}
	} else {
		programLocation = _data->pathRule;
	}

	co_await resume_background();
	Win32Utils::OpenFolderAndSelectFile(programLocation.c_str());
}

std::wstring GetStartFolderForSettingLauncher(const Profile& profile) noexcept {
	if (profile.launcherPath.empty()) {
		// 没有指定启动器
		size_t delimPos = profile.pathRule.find_last_of(L'\\');
		return delimPos == std::wstring::npos ? std::wstring() : profile.pathRule.substr(0, delimPos);
	}

	if (!PathIsRelative(profile.launcherPath.c_str())) {
		// 位于不同的驱动器上
		size_t delimPos = profile.launcherPath.find_last_of(L'\\');
		return delimPos == std::wstring::npos ? std::wstring() : profile.launcherPath.substr(0, delimPos);
	}

	size_t delimPos = profile.pathRule.find_last_of(L'\\');
	if (delimPos == std::wstring::npos) {
		return {};
	}

	// 相对路径转绝对路径
	std::wstring combinedPath(MAX_PATH, 0);
	if (!PathCombine(
		combinedPath.data(),
		profile.pathRule.substr(0, delimPos).c_str(),
		profile.launcherPath.c_str()
	)) {
		return {};
	}

	combinedPath.resize(StrUtils::StrLen(combinedPath.c_str()));
	delimPos = combinedPath.find_last_of(L'\\');
	if (delimPos == std::wstring::npos) {
		return {};
	}

	combinedPath.resize(delimPos);
	return combinedPath;
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

	static std::wstring titleStr(ResourceLoader::GetForCurrentView().GetString(L"SelectLauncherDialog_Title"));
	fileDialog->SetTitle(titleStr.c_str());

	static std::wstring exeFileStr(ResourceLoader::GetForCurrentView().GetString(L"FileDialog_ExeFile"));
	const COMDLG_FILTERSPEC fileType{ exeFileStr.c_str(), L"*.exe"};
	fileDialog->SetFileTypes(1, &fileType);
	fileDialog->SetDefaultExtension(L"exe");

	std::wstring startFolder = GetStartFolderForSettingLauncher(*_data);
	if (!startFolder.empty() && Win32Utils::DirExists(startFolder.c_str())) {
		com_ptr<IShellItem> shellItem;
		SHCreateItemFromParsingName(startFolder.c_str(), nullptr, IID_PPV_ARGS(&shellItem));
		fileDialog->SetFolder(shellItem.get());
	}

	std::optional<std::wstring> exePath = FileDialogHelper::OpenFileDialog(fileDialog.get(), FOS_STRICTFILETYPES);
	if (!exePath || exePath->empty() || *exePath == _data->pathRule) {
		return;
	}

	if (PathIsSameRoot(exePath->c_str(), _data->pathRule.c_str())) {
		// 绝对路径转相对路径
		_data->launcherPath.clear();
		_data->launcherPath.resize(MAX_PATH, 0);
		PathRelativePathTo(
			_data->launcherPath.data(),
			_data->pathRule.c_str(),
			FILE_ATTRIBUTE_NORMAL,
			exePath->c_str(),
			FILE_ATTRIBUTE_NORMAL
		);
		_data->launcherPath.resize(StrUtils::StrLen(_data->launcherPath.c_str()));
	} else {
		// 位于不同的驱动器上
		_data->launcherPath = std::move(*exePath);
	}

	AppSettings::Get().SaveAsync();
}

hstring ProfileViewModel::Name() const noexcept {
	if (_data->name.empty()) {
		return ResourceLoader::GetForCurrentView().GetString(L"Root_Defaults/Content");
	} else {
		return hstring(_data->name);
	}
}

static void LaunchPackagedApp(const Profile& profile) noexcept {
	// 关于启动打包应用的讨论：
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
	if (profile.launcherPath.empty()) {
		Win32Utils::ShellOpen(profile.pathRule.c_str(), profile.launchParameters.c_str());
		return;
	}

	if (!PathIsRelative(profile.launcherPath.c_str())) {
		// 位于不同的驱动器上
		if (Win32Utils::FileExists(profile.launcherPath.c_str())) {
			Win32Utils::ShellOpen(profile.launcherPath.c_str(), profile.launchParameters.c_str());
		} else {
			Win32Utils::ShellOpen(profile.pathRule.c_str(), profile.launchParameters.c_str());
		}
		return;
	}

	size_t delimPos = profile.pathRule.find_last_of(L'\\');
	if (delimPos == std::wstring::npos) {
		return;
	}

	// 相对路径转绝对路径
	wchar_t combinedPath[MAX_PATH];
	if (!PathCombine(combinedPath, profile.pathRule.substr(0, delimPos).c_str(), profile.launcherPath.c_str())
		|| !Win32Utils::FileExists(combinedPath)) {
		Win32Utils::ShellOpen(profile.pathRule.c_str(), profile.launchParameters.c_str());
		return;
	}

	Win32Utils::ShellOpen(combinedPath, profile.launchParameters.c_str());
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
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"RenameText"));

	_trimedRenameText = value;
	StrUtils::Trim(_trimedRenameText);
	bool newEnabled = !_trimedRenameText.empty() && _trimedRenameText != _data->name;
	if (_isRenameConfirmButtonEnabled != newEnabled) {
		_isRenameConfirmButtonEnabled = newEnabled;
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsRenameConfirmButtonEnabled"));
	}
}

void ProfileViewModel::Rename() {
	if (_isDefaultProfile || !_isRenameConfirmButtonEnabled) {
		return;
	}

	ProfileService::Get().RenameProfile(_index, _trimedRenameText);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Name"));
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

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveUp"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveDown"));
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

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveUp"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveDown"));
}

void ProfileViewModel::Delete() {
	if (_isDefaultProfile) {
		return;
	}

	ProfileService::Get().RemoveProfile(_index);
	_data = nullptr;
}

int ProfileViewModel::ScalingMode() const noexcept {
	return _data->scalingMode + 1;
}

void ProfileViewModel::ScalingMode(int value) {
	_data->scalingMode = value - 1;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingMode"));

	AppSettings::Get().SaveAsync();
}

int ProfileViewModel::CaptureMethod() const noexcept {
	if (Win32Utils::GetOSVersion().Is20H1OrNewer() || _data->captureMethod < CaptureMethod::DesktopDuplication) {
		return (int)_data->captureMethod;
	} else {
		return (int)_data->captureMethod - 1;
	}
}

void ProfileViewModel::CaptureMethod(int value) {
	if (value < 0) {
		return;
	}

	if (!Win32Utils::GetOSVersion().Is20H1OrNewer() && value >= (int)CaptureMethod::DesktopDuplication) {
		++value;
	}

	::Magpie::Core::CaptureMethod captureMethod = (::Magpie::Core::CaptureMethod)value;
	if (_data->captureMethod == captureMethod) {
		return;
	}

	_data->captureMethod = captureMethod;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CaptureMethod"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanCaptureTitleBar"));

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
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsAutoScale"));

	AppSettings::Get().SaveAsync();

	if (value) {
		// 立即检查前台窗口是否应自动缩放
		MagService::Get().CheckForeground();
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
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Is3DGameMode"));

	AppSettings::Get().SaveAsync();
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

	::Magpie::Core::MultiMonitorUsage multiMonitorUsage = (::Magpie::Core::MultiMonitorUsage)value;
	if (_data->multiMonitorUsage == multiMonitorUsage) {
		return;
	}

	_data->multiMonitorUsage = multiMonitorUsage;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"MultiMonitorUsage"));

	AppSettings::Get().SaveAsync();
}

IVector<IInspectable> ProfileViewModel::GraphicsCards() const noexcept {
	std::vector<IInspectable> graphicsCards;
	graphicsCards.reserve(_graphicsCards.size() + 1);

	ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView();
	hstring defaultStr = resourceLoader.GetString(L"Profile_General_CaptureMethod_Default");
	graphicsCards.push_back(box_value(defaultStr));

	for (const std::wstring& graphicsCard : _graphicsCards) {
		graphicsCards.push_back(box_value(graphicsCard));
	}

	return single_threaded_vector(std::move(graphicsCards));
}

int ProfileViewModel::GraphicsCard() const noexcept {
	return _data->graphicsCard + 1;
}

void ProfileViewModel::GraphicsCard(int value) {
	if (value < 0) {
		return;
	}

	--value;
	if (_data->graphicsCard == value) {
		return;
	}

	_data->graphicsCard = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"GraphicsCard"));

	AppSettings::Get().SaveAsync();
}

bool ProfileViewModel::IsShowGraphicsCardSettingsCard() const noexcept {
	return _graphicsCards.size() > 1;
}

bool ProfileViewModel::IsShowFPS() const noexcept {
	return _data->IsShowFPS();
}

void ProfileViewModel::IsShowFPS(bool value) {
	if (_data->IsShowFPS() == value) {
		return;
	}

	_data->IsShowFPS(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsShowFPS"));

	AppSettings::Get().SaveAsync();
}

bool ProfileViewModel::IsVSync() const noexcept {
	return _data->IsVSync();
}

void ProfileViewModel::IsVSync(bool value) {
	if (_data->IsVSync() == value) {
		return;
	}

	_data->IsVSync(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsVSync"));

	AppSettings::Get().SaveAsync();
}

bool ProfileViewModel::IsTripleBuffering() const noexcept {
	return _data->IsTripleBuffering();
}

void ProfileViewModel::IsTripleBuffering(bool value) {
	if (_data->IsTripleBuffering() == value) {
		return;
	}

	_data->IsTripleBuffering(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsTripleBuffering"));

	AppSettings::Get().SaveAsync();
}

bool ProfileViewModel::IsDisableWindowResizing() const noexcept {
	return _data->IsDisableWindowResizing();
}

void ProfileViewModel::IsDisableWindowResizing(bool value) {
	if (_data->IsDisableWindowResizing() == value) {
		return;
	}

	_data->IsDisableWindowResizing(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDisableWindowResizing"));

	AppSettings::Get().SaveAsync();
}

bool ProfileViewModel::IsCaptureTitleBar() const noexcept {
	return _data->IsCaptureTitleBar();
}

void ProfileViewModel::IsCaptureTitleBar(bool value) {
	if (_data->IsCaptureTitleBar() == value) {
		return;
	}

	_data->IsCaptureTitleBar(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsCaptureTitleBar"));

	AppSettings::Get().SaveAsync();
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
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsCroppingEnabled"));

	AppSettings::Get().SaveAsync();
}

double ProfileViewModel::CroppingLeft() const noexcept {
	return _data->cropping.Left;
}

void ProfileViewModel::CroppingLeft(double value) {
	if (_data->cropping.Left == value) {
		return;
	}

	_data->cropping.Left = std::isnan(value) ? 0.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingLeft"));

	AppSettings::Get().SaveAsync();
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
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingTop"));

	AppSettings::Get().SaveAsync();
}

double ProfileViewModel::CroppingRight() const noexcept {
	return _data->cropping.Right;
}

void ProfileViewModel::CroppingRight(double value) {
	if (_data->cropping.Right == value) {
		return;
	}

	_data->cropping.Right = std::isnan(value) ? 0.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingRight"));

	AppSettings::Get().SaveAsync();
}

double ProfileViewModel::CroppingBottom() const noexcept {
	return _data->cropping.Bottom;
}

void ProfileViewModel::CroppingBottom(double value) {
	if (_data->cropping.Bottom == value) {
		return;
	}

	_data->cropping.Bottom = std::isnan(value) ? 0.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingBottom"));

	AppSettings::Get().SaveAsync();
}

bool ProfileViewModel::IsAdjustCursorSpeed() const noexcept {
	return _data->IsAdjustCursorSpeed();
}

void ProfileViewModel::IsAdjustCursorSpeed(bool value) {
	if (_data->IsAdjustCursorSpeed() == value) {
		return;
	}

	_data->IsAdjustCursorSpeed(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsAdjustCursorSpeed"));

	AppSettings::Get().SaveAsync();
}

bool ProfileViewModel::IsDrawCursor() const noexcept {
	return _data->IsDrawCursor();
}

void ProfileViewModel::IsDrawCursor(bool value) {
	if (_data->IsDrawCursor() == value) {
		return;
	}

	_data->IsDrawCursor(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDrawCursor"));

	AppSettings::Get().SaveAsync();
}

int ProfileViewModel::CursorScaling() const noexcept {
	return (int)_data->cursorScaling;
}

void ProfileViewModel::CursorScaling(int value) {
	if (value < 0) {
		return;
	}

	Magpie::App::CursorScaling cursorScaling = (Magpie::App::CursorScaling)value;
	if (_data->cursorScaling == cursorScaling) {
		return;
	}

	_data->cursorScaling = cursorScaling;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CursorScaling"));

	AppSettings::Get().SaveAsync();
}

double ProfileViewModel::CustomCursorScaling() const noexcept {
	return _data->customCursorScaling;
}

void ProfileViewModel::CustomCursorScaling(double value) {
	if (_data->customCursorScaling == value) {
		return;
	}

	_data->customCursorScaling = std::isnan(value) ? 1.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CustomCursorScaling"));

	AppSettings::Get().SaveAsync();
}

int ProfileViewModel::CursorInterpolationMode() const noexcept {
	return (int)_data->cursorInterpolationMode;
}

void ProfileViewModel::CursorInterpolationMode(int value) {
	if (value < 0) {
		return;
	}

	::Magpie::Core::CursorInterpolationMode cursorInterpolationMode = (::Magpie::Core::CursorInterpolationMode)value;
	if (_data->cursorInterpolationMode == cursorInterpolationMode) {
		return;
	}

	_data->cursorInterpolationMode = cursorInterpolationMode;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CursorInterpolationMode"));

	AppSettings::Get().SaveAsync();
}

hstring ProfileViewModel::LaunchParameters() const noexcept {
	return hstring(_data->launchParameters);
}

void ProfileViewModel::LaunchParameters(const hstring& value) {
	std::wstring_view trimmed(value);
	StrUtils::Trim(trimmed);
	_data->launchParameters = trimmed;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"LaunchParameters"));

	AppSettings::Get().SaveAsync();
}

void ProfileViewModel::IsEditingLaunchParameters(bool value) {
	if (_isEditingLaunchParameters == value) {
		return;
	}

	_isEditingLaunchParameters = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsEditingLaunchParameters"));
}

bool ProfileViewModel::IsDisableDirectFlip() const noexcept {
	return _data->IsDisableDirectFlip();
}

void ProfileViewModel::IsDisableDirectFlip(bool value) {
	if (_data->IsDisableDirectFlip() == value) {
		return;
	}

	_data->IsDisableDirectFlip(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDisableDirectFlip"));

	AppSettings::Get().SaveAsync();
}

fire_and_forget ProfileViewModel::_LoadIcon(FrameworkElement const& rootPage) {
	std::wstring iconPath;
	SoftwareBitmap iconBitmap{ nullptr };

	if (_isProgramExist) {
		auto weakThis = get_weak();

		const bool preferLightTheme = rootPage.ActualTheme() == ElementTheme::Light;
		const bool isPackaged = _data->isPackaged;
		const std::wstring path = _data->pathRule;
		CoreDispatcher dispatcher = rootPage.Dispatcher();
		const uint32_t dpi = (uint32_t)std::lroundf(_displayInformation.LogicalDpi());

		co_await resume_background();

		static constexpr const UINT ICON_SIZE = 32;
		if (isPackaged) {
			AppXReader appxReader;
			[[maybe_unused]] bool result = appxReader.Initialize(path);
			assert(result);

			std::variant<std::wstring, SoftwareBitmap> uwpIcon =
				appxReader.GetIcon((uint32_t)std::ceil(dpi * ICON_SIZE / double(USER_DEFAULT_SCREEN_DPI)), preferLightTheme);
			if (uwpIcon.index() == 0) {
				iconPath = std::get<0>(uwpIcon);
			} else {
				iconBitmap = std::get<1>(uwpIcon);
			}
		} else {
			iconBitmap = IconHelper::ExtractIconFromExe(path.c_str(), ICON_SIZE, dpi);
		}

		co_await dispatcher;
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

		MUXC::ImageIcon imageIcon;
		imageIcon.Source(imageSource);

		_icon = std::move(imageIcon);
	} else {
		_icon = nullptr;
	}

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Icon"));
}

}
