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


using namespace winrt;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace ::Magpie::Core;


namespace winrt::Magpie::UI::implementation {

static std::vector<std::wstring> GetAllGraphicsAdapters() {
	com_ptr<IDXGIFactory1> dxgiFactory;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		return {};
	}

	std::vector<std::wstring> result;

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

ScalingProfileViewModel::ScalingProfileViewModel(int32_t profileIdx) : _isDefaultProfile(profileIdx < 0) {
	if (_isDefaultProfile) {
		_profile = &ScalingProfileService::Get().DefaultScalingProfile();
	} else {
		_profileIdx = (uint32_t)profileIdx;
		_profile = &ScalingProfileService::Get().GetProfile(profileIdx);

		MUXC::ImageIcon placeholderIcon;
		_icon = std::move(placeholderIcon);

		App app = Application::Current().as<App>();
		MainPage mainPage = app.MainPage();
		_themeChangedRevoker = mainPage.ActualThemeChanged(
			auto_revoke,
			[this](FrameworkElement const& sender, IInspectable const&) {
				_LoadIcon(sender);
			}
		);

		_displayInformation = app.DisplayInformation();
		_dpiChangedRevoker = _displayInformation.DpiChanged(
			auto_revoke,
			[this](DisplayInformation const&, IInspectable const&) {
				// 不能捕获 mainPage，会引起内存泄露
				_LoadIcon(Application::Current().as<App>().MainPage());
			}
		);

		_LoadIcon(mainPage);
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

bool ScalingProfileViewModel::IsNotDefaultScalingProfile() const noexcept {
	return !_profile->name.empty();
}

bool ScalingProfileViewModel::IsOpenProgramLocationMenuEnabled() const noexcept {
	return Win32Utils::FileExists(_profile->pathRule.c_str());
}

fire_and_forget ScalingProfileViewModel::OpenProgramLocation() const noexcept {
	if (!Win32Utils::FileExists(_profile->pathRule.c_str())) {
		co_return;
	}
	std::wstring programLocation = _profile->pathRule;

	// 根据 https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shparsedisplayname，
	// SHParseDisplayName 不能在主线程调用
	co_await resume_background();

	PIDLIST_ABSOLUTE pidl;
	HRESULT hr = SHParseDisplayName(programLocation.c_str(), nullptr, &pidl, 0, nullptr);
	if (FAILED(hr)) {
		Logger::Get().ComError("SHParseDisplayName 失败", hr);
		co_return;
	}

	SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);

	CoTaskMemFree(pidl);
}

hstring ScalingProfileViewModel::Name() const noexcept {
	return hstring(_profile->name.empty() ? L"默认" : _profile->name);
}

void ScalingProfileViewModel::RenameText(const hstring& value) {
	_renameText = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"RenameText"));

	_trimedRenameText = value;
	StrUtils::Trim(_trimedRenameText);
	bool newEnabled = !_trimedRenameText.empty() && _trimedRenameText != _profile->name;
	if (_isRenameConfirmButtonEnabled != newEnabled) {
		_isRenameConfirmButtonEnabled = newEnabled;
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsRenameConfirmButtonEnabled"));
	}
}

void ScalingProfileViewModel::Rename() {
	if (_isDefaultProfile || !_isRenameConfirmButtonEnabled) {
		return;
	}

	ScalingProfileService::Get().RenameProfile(_profileIdx, _trimedRenameText);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Name"));
}

bool ScalingProfileViewModel::CanMoveUp() const noexcept {
	return !_isDefaultProfile && _profileIdx != 0;
}

bool ScalingProfileViewModel::CanMoveDown() const noexcept {
	return !_isDefaultProfile && _profileIdx + 1 < ScalingProfileService::Get().GetProfileCount();
}

void ScalingProfileViewModel::MoveUp() {
	if (_isDefaultProfile) {
		return;
	}

	ScalingProfileService& scalingProfileService = ScalingProfileService::Get();
	if (!scalingProfileService.ReorderProfile(_profileIdx, true)) {
		return;
	}

	--_profileIdx;
	_profile = &scalingProfileService.GetProfile(_profileIdx);

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveUp"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveDown"));
}

void ScalingProfileViewModel::MoveDown() {
	if (_isDefaultProfile) {
		return;
	}

	ScalingProfileService& scalingProfileService = ScalingProfileService::Get();
	if (!scalingProfileService.ReorderProfile(_profileIdx, false)) {
		return;
	}

	++_profileIdx;
	_profile = &scalingProfileService.GetProfile(_profileIdx);

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveUp"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveDown"));
}

void ScalingProfileViewModel::Delete() {
	if (_isDefaultProfile) {
		return;
	}

	ScalingProfileService::Get().RemoveProfile(_profileIdx);
	_profile = nullptr;
}

int32_t ScalingProfileViewModel::CaptureMode() const noexcept {
	return (int32_t)_profile->captureMode;
}

void ScalingProfileViewModel::CaptureMode(int32_t value) {
	if (value < 0) {
		return;
	}

	::Magpie::Core::CaptureMode captureMode = (::Magpie::Core::CaptureMode)value;
	if (_profile->captureMode == captureMode) {
		return;
	}

	_profile->captureMode = captureMode;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CaptureMode"));
}

bool ScalingProfileViewModel::Is3DGameMode() const noexcept {
	return _profile->Is3DGameMode();
}

void ScalingProfileViewModel::Is3DGameMode(bool value) {
	if (_profile->Is3DGameMode() == value) {
		return;
	}

	_profile->Is3DGameMode(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Is3DGameMode"));
}

int32_t ScalingProfileViewModel::MultiMonitorUsage() const noexcept {
	return (int32_t)_profile->multiMonitorUsage;
}

void ScalingProfileViewModel::MultiMonitorUsage(int32_t value) {
	if (value < 0) {
		return;
	}

	::Magpie::Core::MultiMonitorUsage multiMonitorUsage = (::Magpie::Core::MultiMonitorUsage)value;
	if (_profile->multiMonitorUsage == multiMonitorUsage) {
		return;
	}

	_profile->multiMonitorUsage = multiMonitorUsage;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"MultiMonitorUsage"));
}

int32_t ScalingProfileViewModel::GraphicsAdapter() const noexcept {
	return (int32_t)_profile->graphicsAdapter;
}

void ScalingProfileViewModel::GraphicsAdapter(int32_t value) {
	if (value < 0) {
		return;
	}

	uint32_t graphicsAdapter = (uint32_t)value;
	if (_profile->graphicsAdapter == graphicsAdapter) {
		return;
	}

	_profile->graphicsAdapter = graphicsAdapter;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"GraphicsAdapter"));
}

bool ScalingProfileViewModel::IsShowFPS() const noexcept {
	return _profile->IsShowFPS();
}

void ScalingProfileViewModel::IsShowFPS(bool value) {
	if (_profile->IsShowFPS() == value) {
		return;
	}

	_profile->IsShowFPS(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsShowFPS"));
}

bool ScalingProfileViewModel::IsVSync() const noexcept {
	return _profile->IsVSync();
}

void ScalingProfileViewModel::IsVSync(bool value) {
	if (_profile->IsVSync() == value) {
		return;
	}

	_profile->IsVSync(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsVSync"));
}

bool ScalingProfileViewModel::IsTripleBuffering() const noexcept {
	return _profile->IsTripleBuffering();
}

void ScalingProfileViewModel::IsTripleBuffering(bool value) {
	if (_profile->IsTripleBuffering() == value) {
		return;
	}

	_profile->IsTripleBuffering(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsTripleBuffering"));
}

bool ScalingProfileViewModel::IsDisableWindowResizing() const noexcept {
	return _profile->IsDisableWindowResizing();
}

void ScalingProfileViewModel::IsDisableWindowResizing(bool value) {
	if (_profile->IsDisableWindowResizing() == value) {
		return;
	}

	_profile->IsDisableWindowResizing(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDisableWindowResizing"));
}

bool ScalingProfileViewModel::IsReserveTitleBar() const noexcept {
	return _profile->IsReserveTitleBar();
}

void ScalingProfileViewModel::IsReserveTitleBar(bool value) {
	if (_profile->IsReserveTitleBar() == value) {
		return;
	}

	_profile->IsReserveTitleBar(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsReserveTitleBar"));
}

bool ScalingProfileViewModel::IsCroppingEnabled() const noexcept {
	return _profile->isCroppingEnabled;
}

void ScalingProfileViewModel::IsCroppingEnabled(bool value) {
	if (_profile->isCroppingEnabled == value) {
		return;
	}

	_profile->isCroppingEnabled = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsCroppingEnabled"));
}

double ScalingProfileViewModel::CroppingLeft() const noexcept {
	return _profile->cropping.Left;
}

void ScalingProfileViewModel::CroppingLeft(double value) {
	if (_profile->cropping.Left == value) {
		return;
	}

	_profile->cropping.Left = std::isnan(value) ? 0.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingLeft"));
}

double ScalingProfileViewModel::CroppingTop() const noexcept {
	return _profile->cropping.Top;
}

void ScalingProfileViewModel::CroppingTop(double value) {
	if (_profile->cropping.Top == value) {
		return;
	}

	// 用户已清空数字框则重置为 0
	_profile->cropping.Top = std::isnan(value) ? 0.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingTop"));
}

double ScalingProfileViewModel::CroppingRight() const noexcept {
	return _profile->cropping.Right;
}

void ScalingProfileViewModel::CroppingRight(double value) {
	if (_profile->cropping.Right == value) {
		return;
	}

	_profile->cropping.Right = std::isnan(value) ? 0.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingRight"));
}

double ScalingProfileViewModel::CroppingBottom() const noexcept {
	return _profile->cropping.Bottom;
}

void ScalingProfileViewModel::CroppingBottom(double value) {
	if (_profile->cropping.Bottom == value) {
		return;
	}

	_profile->cropping.Bottom = std::isnan(value) ? 0.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingBottom"));
}

bool ScalingProfileViewModel::IsAdjustCursorSpeed() const noexcept {
	return _profile->IsAdjustCursorSpeed();
}

void ScalingProfileViewModel::IsAdjustCursorSpeed(bool value) {
	if (_profile->IsAdjustCursorSpeed() == value) {
		return;
	}

	_profile->IsAdjustCursorSpeed(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsAdjustCursorSpeed"));
}

bool ScalingProfileViewModel::IsDrawCursor() const noexcept {
	return _profile->IsDrawCursor();
}

void ScalingProfileViewModel::IsDrawCursor(bool value) {
	if (_profile->IsDrawCursor() == value) {
		return;
	}

	_profile->IsDrawCursor(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDrawCursor"));
}

int32_t ScalingProfileViewModel::CursorScaling() const noexcept {
	return (int32_t)_profile->cursorScaling;
}

void ScalingProfileViewModel::CursorScaling(int32_t value) {
	if (value < 0) {
		return;
	}

	Magpie::UI::CursorScaling cursorScaling = (Magpie::UI::CursorScaling)value;
	if (_profile->cursorScaling == cursorScaling) {
		return;
	}

	_profile->cursorScaling = cursorScaling;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CursorScaling"));
}

double ScalingProfileViewModel::CustomCursorScaling() const noexcept {
	return _profile->customCursorScaling;
}

void ScalingProfileViewModel::CustomCursorScaling(double value) {
	if (_profile->customCursorScaling == value) {
		return;
	}

	_profile->customCursorScaling = std::isnan(value) ? 1.0f : (float)value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CustomCursorScaling"));
}

int32_t ScalingProfileViewModel::CursorInterpolationMode() const noexcept {
	return (int32_t)_profile->cursorInterpolationMode;
}

void ScalingProfileViewModel::CursorInterpolationMode(int32_t value) {
	if (value < 0) {
		return;
	}

	::Magpie::Core::CursorInterpolationMode cursorInterpolationMode = (::Magpie::Core::CursorInterpolationMode)value;
	if (_profile->cursorInterpolationMode == cursorInterpolationMode) {
		return;
	}

	_profile->cursorInterpolationMode = cursorInterpolationMode;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CursorInterpolationMode"));
}

bool ScalingProfileViewModel::IsDisableDirectFlip() const noexcept {
	return _profile->IsDisableDirectFlip();
}

void ScalingProfileViewModel::IsDisableDirectFlip(bool value) {
	if (_profile->IsDisableDirectFlip() == value) {
		return;
	}

	_profile->IsDisableDirectFlip(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDisableDirectFlip"));
}

fire_and_forget ScalingProfileViewModel::_LoadIcon(FrameworkElement const& mainPage) {
	auto weakThis = get_weak();

	bool preferLightTheme = mainPage.ActualTheme() == ElementTheme::Light;
	bool isPackaged = _profile->isPackaged;
	std::wstring path = _profile->pathRule;
	CoreDispatcher dispatcher = mainPage.Dispatcher();
	uint32_t dpi = (uint32_t)std::lroundf(DisplayInformation::GetForCurrentView().LogicalDpi());

	co_await resume_background();

	std::wstring iconPath;
	SoftwareBitmap iconBitmap{ nullptr };

	static constexpr const UINT ICON_SIZE = 32;
	if (isPackaged) {
		AppXReader reader;
		reader.Initialize(path);

		std::variant<std::wstring, SoftwareBitmap> uwpIcon =
			reader.GetIcon((uint32_t)std::ceil(dpi * ICON_SIZE / 96.0), preferLightTheme);
		if (uwpIcon.index() == 0) {
			iconPath = std::get<0>(uwpIcon);
		} else {
			iconBitmap = std::get<1>(uwpIcon);
		}
	} else {
		iconBitmap = IconHelper::GetIconOfExe(path.c_str(), ICON_SIZE, dpi);
	}

	co_await dispatcher;

	auto strongRef = weakThis.get();
	if (!strongRef) {
		co_return;
	}

	if (!iconPath.empty()) {
		BitmapIcon icon;
		icon.ShowAsMonochrome(false);
		icon.UriSource(Uri(iconPath));

		strongRef->_icon = std::move(icon);
	} else if (iconBitmap) {
		SoftwareBitmapSource imageSource;
		co_await imageSource.SetBitmapAsync(iconBitmap);

		MUXC::ImageIcon imageIcon;
		imageIcon.Source(imageSource);

		strongRef->_icon = std::move(imageIcon);
	} else {
		FontIcon icon;
		icon.Glyph(L"\uECAA");
		strongRef->_icon = std::move(icon);
	}

	strongRef->_propertyChangedEvent(*strongRef, PropertyChangedEventArgs(L"Icon"));
}

}
