#include "pch.h"
#include "ScalingProfileViewModel.h"
#if __has_include("ScalingProfileViewModel.g.cpp")
#include "ScalingProfileViewModel.g.cpp"
#endif
#include "ScalingProfile.h"
#include "AppXReader.h"
#include "IconHelper.h"
#include "ScalingProfileService.h"
#include "StrUtils.h"
#include <Magpie.Core.h>
#include "Win32Utils.h"
#include "AppSettings.h"
#include "Logger.h"
#include "ScalingMode.h"
#include <dxgi.h>

using namespace winrt;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace ::Magpie::Core;

namespace winrt::Magpie::App::implementation {

static SmallVector<std::wstring, 2> GetAllGraphicsAdapters() {
	com_ptr<IDXGIFactory1> dxgiFactory;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		return {};
	}

	SmallVector<std::wstring, 2> result;

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

ScalingProfileViewModel::ScalingProfileViewModel(int profileIdx) : _isDefaultProfile(profileIdx < 0) {
	if (_isDefaultProfile) {
		_data = &ScalingProfileService::Get().DefaultScalingProfile();
	} else {
		_index = (uint32_t)profileIdx;
		_data = &ScalingProfileService::Get().GetProfile(profileIdx);

		// 占位
		_icon = FontIcon();

		App app = Application::Current().as<App>();
		MainPage mainPage = app.MainPage();
		_themeChangedRevoker = mainPage.ActualThemeChanged(
			auto_revoke,
			[this](FrameworkElement const& sender, IInspectable const&) {
				_LoadIcon(sender);
			}
		);

		_displayInformation = DisplayInformation::GetForCurrentView();
		_dpiChangedRevoker = _displayInformation.DpiChanged(
			auto_revoke,
			[this](DisplayInformation const&, IInspectable const&) {
				if (MainPage mainPage = Application::Current().as<App>().MainPage()) {
					_LoadIcon(mainPage);
				}
			}
		);

		if (_data->isPackaged) {
			AppXReader appxReader;
			_isProgramExist = appxReader.Initialize(_data->pathRule);
		} else {
			_isProgramExist = Win32Utils::FileExists(_data->pathRule.c_str());
		}

		_LoadIcon(mainPage);
	}

	{
		std::vector<IInspectable> scalingModes;
		scalingModes.push_back(box_value(L"无"));
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

	std::vector<IInspectable> graphicsAdapters;
	graphicsAdapters.push_back(box_value(L"默认"));

	for (const std::wstring& adapter : GetAllGraphicsAdapters()) {
		graphicsAdapters.push_back(box_value(adapter));
	}

	if (graphicsAdapters.size() <= 2 || GraphicsAdapter() >= graphicsAdapters.size()) {
		GraphicsAdapter(0);
	}

	_graphicsAdapters = single_threaded_vector(std::move(graphicsAdapters));
}

ScalingProfileViewModel::~ScalingProfileViewModel() {}

bool ScalingProfileViewModel::IsNotDefaultScalingProfile() const noexcept {
	return !_data->name.empty();
}

fire_and_forget ScalingProfileViewModel::OpenProgramLocation() const noexcept {
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

hstring ScalingProfileViewModel::Name() const noexcept {
	return hstring(_data->name.empty() ? L"默认" : _data->name);
}

static void LaunchPackagedApp(const wchar_t* aumid) {
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
	hr = aam->ActivateApplication(aumid, nullptr, AO_NONE, &procId);
	if (FAILED(hr)) {
		Logger::Get().ComError("IApplicationActivationManager::ActivateApplication 失败", hr);
		return;
	}
}

void ScalingProfileViewModel::Launch() const noexcept {
	if (!_isProgramExist) {
		return;
	}

	if (_data->isPackaged) {
		LaunchPackagedApp(_data->pathRule.c_str());
	} else {
		Win32Utils::ShellOpen(_data->pathRule.c_str());
	}
}

void ScalingProfileViewModel::RenameText(const hstring& value) {
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

void ScalingProfileViewModel::Rename() {
	if (_isDefaultProfile || !_isRenameConfirmButtonEnabled) {
		return;
	}

	ScalingProfileService::Get().RenameProfile(_index, _trimedRenameText);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Name"));
}

bool ScalingProfileViewModel::CanMoveUp() const noexcept {
	return !_isDefaultProfile && _index != 0;
}

bool ScalingProfileViewModel::CanMoveDown() const noexcept {
	return !_isDefaultProfile && _index + 1 < ScalingProfileService::Get().GetProfileCount();
}

void ScalingProfileViewModel::MoveUp() {
	if (_isDefaultProfile) {
		return;
	}

	ScalingProfileService& scalingProfileService = ScalingProfileService::Get();
	if (!scalingProfileService.MoveProfile(_index, true)) {
		return;
	}

	--_index;
	_data = &scalingProfileService.GetProfile(_index);

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveUp"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveDown"));
}

void ScalingProfileViewModel::MoveDown() {
	if (_isDefaultProfile) {
		return;
	}

	ScalingProfileService& scalingProfileService = ScalingProfileService::Get();
	if (!scalingProfileService.MoveProfile(_index, false)) {
		return;
	}

	++_index;
	_data = &scalingProfileService.GetProfile(_index);

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveUp"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveDown"));
}

void ScalingProfileViewModel::Delete() {
	if (_isDefaultProfile) {
		return;
	}

	ScalingProfileService::Get().RemoveProfile(_index);
	_data = nullptr;
}

int ScalingProfileViewModel::ScalingMode() const noexcept {
	return _data->scalingMode + 1;
}

void ScalingProfileViewModel::ScalingMode(int value) {
	_data->scalingMode = value - 1;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingMode"));

	AppSettings::Get().SaveAsync();
}

int ScalingProfileViewModel::CaptureMethod() const noexcept {
	if (Win32Utils::GetOSVersion().Is20H1OrNewer() || _data->captureMethod < CaptureMethod::DesktopDuplication) {
		return (int)_data->captureMethod;
	} else {
		return (int)_data->captureMethod - 1;
	}
}

void ScalingProfileViewModel::CaptureMethod(int value) {
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

	AppSettings::Get().SaveAsync();
}

bool ScalingProfileViewModel::Is3DGameMode() const noexcept {
	return _data->Is3DGameMode();
}

void ScalingProfileViewModel::Is3DGameMode(bool value) {
	if (_data->Is3DGameMode() == value) {
		return;
	}

	_data->Is3DGameMode(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Is3DGameMode"));

	AppSettings::Get().SaveAsync();
}

int ScalingProfileViewModel::MultiMonitorUsage() const noexcept {
	return (int)_data->multiMonitorUsage;
}

void ScalingProfileViewModel::MultiMonitorUsage(int value) {
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

int ScalingProfileViewModel::GraphicsAdapter() const noexcept {
	return (int)_data->graphicsAdapter;
}

void ScalingProfileViewModel::GraphicsAdapter(int value) {
	if (value < 0) {
		return;
	}

	uint32_t graphicsAdapter = (uint32_t)value;
	if (_data->graphicsAdapter == graphicsAdapter) {
		return;
	}

	_data->graphicsAdapter = graphicsAdapter;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"GraphicsAdapter"));

	AppSettings::Get().SaveAsync();
}

bool ScalingProfileViewModel::IsShowFPS() const noexcept {
	return _data->IsShowFPS();
}

void ScalingProfileViewModel::IsShowFPS(bool value) {
	if (_data->IsShowFPS() == value) {
		return;
	}

	_data->IsShowFPS(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsShowFPS"));

	AppSettings::Get().SaveAsync();
}

bool ScalingProfileViewModel::IsVSync() const noexcept {
	return _data->IsVSync();
}

void ScalingProfileViewModel::IsVSync(bool value) {
	if (_data->IsVSync() == value) {
		return;
	}

	_data->IsVSync(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsVSync"));

	AppSettings::Get().SaveAsync();
}

bool ScalingProfileViewModel::IsTripleBuffering() const noexcept {
	return _data->IsTripleBuffering();
}

void ScalingProfileViewModel::IsTripleBuffering(bool value) {
	if (_data->IsTripleBuffering() == value) {
		return;
	}

	_data->IsTripleBuffering(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsTripleBuffering"));

	AppSettings::Get().SaveAsync();
}

bool ScalingProfileViewModel::IsDisableWindowResizing() const noexcept {
	return _data->IsDisableWindowResizing();
}

void ScalingProfileViewModel::IsDisableWindowResizing(bool value) {
	if (_data->IsDisableWindowResizing() == value) {
		return;
	}

	_data->IsDisableWindowResizing(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDisableWindowResizing"));

	AppSettings::Get().SaveAsync();
}

bool ScalingProfileViewModel::IsReserveTitleBar() const noexcept {
	return _data->IsReserveTitleBar();
}

void ScalingProfileViewModel::IsReserveTitleBar(bool value) {
	if (_data->IsReserveTitleBar() == value) {
		return;
	}

	_data->IsReserveTitleBar(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsReserveTitleBar"));

	AppSettings::Get().SaveAsync();
}

bool ScalingProfileViewModel::IsCroppingEnabled() const noexcept {
	return _data->isCroppingEnabled;
}

void ScalingProfileViewModel::IsCroppingEnabled(bool value) {
	if (_data->isCroppingEnabled == value) {
		return;
	}

	_data->isCroppingEnabled = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsCroppingEnabled"));

	AppSettings::Get().SaveAsync();
}

double ScalingProfileViewModel::CroppingLeft() const noexcept {
	return _data->cropping.Left;
}

void ScalingProfileViewModel::CroppingLeft(double value) {
	if (_data->cropping.Left == value) {
		return;
	}

	_data->cropping.Left = std::isnan(value) ? 0.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingLeft"));

	AppSettings::Get().SaveAsync();
}

double ScalingProfileViewModel::CroppingTop() const noexcept {
	return _data->cropping.Top;
}

void ScalingProfileViewModel::CroppingTop(double value) {
	if (_data->cropping.Top == value) {
		return;
	}

	// 用户已清空数字框则重置为 0
	_data->cropping.Top = std::isnan(value) ? 0.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingTop"));

	AppSettings::Get().SaveAsync();
}

double ScalingProfileViewModel::CroppingRight() const noexcept {
	return _data->cropping.Right;
}

void ScalingProfileViewModel::CroppingRight(double value) {
	if (_data->cropping.Right == value) {
		return;
	}

	_data->cropping.Right = std::isnan(value) ? 0.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingRight"));

	AppSettings::Get().SaveAsync();
}

double ScalingProfileViewModel::CroppingBottom() const noexcept {
	return _data->cropping.Bottom;
}

void ScalingProfileViewModel::CroppingBottom(double value) {
	if (_data->cropping.Bottom == value) {
		return;
	}

	_data->cropping.Bottom = std::isnan(value) ? 0.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingBottom"));

	AppSettings::Get().SaveAsync();
}

bool ScalingProfileViewModel::IsAdjustCursorSpeed() const noexcept {
	return _data->IsAdjustCursorSpeed();
}

void ScalingProfileViewModel::IsAdjustCursorSpeed(bool value) {
	if (_data->IsAdjustCursorSpeed() == value) {
		return;
	}

	_data->IsAdjustCursorSpeed(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsAdjustCursorSpeed"));

	AppSettings::Get().SaveAsync();
}

bool ScalingProfileViewModel::IsDrawCursor() const noexcept {
	return _data->IsDrawCursor();
}

void ScalingProfileViewModel::IsDrawCursor(bool value) {
	if (_data->IsDrawCursor() == value) {
		return;
	}

	_data->IsDrawCursor(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDrawCursor"));

	AppSettings::Get().SaveAsync();
}

int ScalingProfileViewModel::CursorScaling() const noexcept {
	return (int)_data->cursorScaling;
}

void ScalingProfileViewModel::CursorScaling(int value) {
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

double ScalingProfileViewModel::CustomCursorScaling() const noexcept {
	return _data->customCursorScaling;
}

void ScalingProfileViewModel::CustomCursorScaling(double value) {
	if (_data->customCursorScaling == value) {
		return;
	}

	_data->customCursorScaling = std::isnan(value) ? 1.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CustomCursorScaling"));

	AppSettings::Get().SaveAsync();
}

int ScalingProfileViewModel::CursorInterpolationMode() const noexcept {
	return (int)_data->cursorInterpolationMode;
}

void ScalingProfileViewModel::CursorInterpolationMode(int value) {
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

bool ScalingProfileViewModel::IsDisableDirectFlip() const noexcept {
	return _data->IsDisableDirectFlip();
}

void ScalingProfileViewModel::IsDisableDirectFlip(bool value) {
	if (_data->IsDisableDirectFlip() == value) {
		return;
	}

	_data->IsDisableDirectFlip(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDisableDirectFlip"));

	AppSettings::Get().SaveAsync();
}

fire_and_forget ScalingProfileViewModel::_LoadIcon(FrameworkElement const& mainPage) {
	std::wstring iconPath;
	SoftwareBitmap iconBitmap{ nullptr };

	if (_isProgramExist) {
		auto weakThis = get_weak();

		const bool preferLightTheme = mainPage.ActualTheme() == ElementTheme::Light;
		const bool isPackaged = _data->isPackaged;
		const std::wstring path = _data->pathRule;
		CoreDispatcher dispatcher = mainPage.Dispatcher();
		const uint32_t dpi = (uint32_t)std::lroundf(_displayInformation.LogicalDpi());

		co_await resume_background();

		static constexpr const UINT ICON_SIZE = 32;
		if (isPackaged) {
			AppXReader appxReader;
			[[maybe_unused]] bool result = appxReader.Initialize(path);
			assert(result);

			std::variant<std::wstring, SoftwareBitmap> uwpIcon =
				appxReader.GetIcon((uint32_t)std::ceil(dpi * ICON_SIZE / 96.0), preferLightTheme);
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
