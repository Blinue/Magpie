#include "pch.h"
#include "ScalingProfileViewModel.h"
#if __has_include("ScalingProfileViewModel.g.cpp")
#include "ScalingProfileViewModel.g.cpp"
#endif
#include "ScalingProfile.h"
#include <dxgi1_6.h>
#include "AppXReader.h"
#include "IconHelper.h"
#include "ScalingProfileService.h"
#include "StrUtils.h"


using namespace winrt;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media::Imaging;


namespace winrt::Magpie::App::implementation {

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
			[this, mainPage](DisplayInformation const&, IInspectable const&) {
			_LoadIcon(mainPage);
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
	return !_profile->Name().empty();
}

hstring ScalingProfileViewModel::Name() const noexcept {
	return hstring(_profile->Name().empty() ? L"默认" : _profile->Name());
}

void ScalingProfileViewModel::RenameText(const hstring& value) {
	_renameText = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"RenameText"));

	_trimedRenameText = value;
	StrUtils::Trim(_trimedRenameText);
	bool newEnabled = !_trimedRenameText.empty() && _trimedRenameText != _profile->Name();
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
}

void ScalingProfileViewModel::Delete() {
	if (_isDefaultProfile) {
		return;
	}

	ScalingProfileService::Get().RemoveProfile(_profileIdx);
	_profile = nullptr;
}

int32_t ScalingProfileViewModel::CaptureMode() const noexcept {
	return (int32_t)_profile->MagSettings().CaptureMode();
}

void ScalingProfileViewModel::CaptureMode(int32_t value) {
	if (value < 0) {
		return;
	}

	Magpie::Runtime::CaptureMode captureMode = (Magpie::Runtime::CaptureMode)value;
	if (_profile->MagSettings().CaptureMode() == captureMode) {
		return;
	}

	_profile->MagSettings().CaptureMode(captureMode);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CaptureMode"));
}

bool ScalingProfileViewModel::Is3DGameMode() const noexcept {
	return _profile->MagSettings().Is3DGameMode();
}

void ScalingProfileViewModel::Is3DGameMode(bool value) {
	if (_profile->MagSettings().Is3DGameMode() == value) {
		return;
	}

	_profile->MagSettings().Is3DGameMode(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Is3DGameMode"));
}

int32_t ScalingProfileViewModel::MultiMonitorUsage() const noexcept {
	return (int32_t)_profile->MagSettings().MultiMonitorUsage();
}

void ScalingProfileViewModel::MultiMonitorUsage(int32_t value) {
	if (value < 0) {
		return;
	}

	Magpie::Runtime::MultiMonitorUsage multiMonitorUsage = (Magpie::Runtime::MultiMonitorUsage)value;
	if (_profile->MagSettings().MultiMonitorUsage() == multiMonitorUsage) {
		return;
	}

	_profile->MagSettings().MultiMonitorUsage(multiMonitorUsage);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"MultiMonitorUsage"));
}

int32_t ScalingProfileViewModel::GraphicsAdapter() const noexcept {
	return (int32_t)_profile->MagSettings().GraphicsAdapter();
}

void ScalingProfileViewModel::GraphicsAdapter(int32_t value) {
	if (value < 0) {
		return;
	}

	uint32_t graphicsAdapter = (uint32_t)value;
	if (_profile->MagSettings().GraphicsAdapter() == graphicsAdapter) {
		return;
	}

	_profile->MagSettings().GraphicsAdapter(graphicsAdapter);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"GraphicsAdapter"));
}

bool ScalingProfileViewModel::IsShowFPS() const noexcept {
	return _profile->MagSettings().IsShowFPS();
}

void ScalingProfileViewModel::IsShowFPS(bool value) {
	if (_profile->MagSettings().IsShowFPS() == value) {
		return;
	}

	_profile->MagSettings().IsShowFPS(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsShowFPS"));
}

bool ScalingProfileViewModel::IsVSync() const noexcept {
	return _profile->MagSettings().IsVSync();
}

void ScalingProfileViewModel::IsVSync(bool value) {
	if (_profile->MagSettings().IsVSync() == value) {
		return;
	}

	_profile->MagSettings().IsVSync(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsVSync"));
}

bool ScalingProfileViewModel::IsTripleBuffering() const noexcept {
	return _profile->MagSettings().IsTripleBuffering();
}

void ScalingProfileViewModel::IsTripleBuffering(bool value) {
	if (_profile->MagSettings().IsTripleBuffering() == value) {
		return;
	}

	_profile->MagSettings().IsTripleBuffering(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsTripleBuffering"));
}

bool ScalingProfileViewModel::IsDisableWindowResizing() const noexcept {
	return _profile->MagSettings().IsDisableWindowResizing();
}

void ScalingProfileViewModel::IsDisableWindowResizing(bool value) {
	if (_profile->MagSettings().IsDisableWindowResizing() == value) {
		return;
	}

	_profile->MagSettings().IsDisableWindowResizing(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDisableWindowResizing"));
}

bool ScalingProfileViewModel::IsReserveTitleBar() const noexcept {
	return _profile->MagSettings().IsReserveTitleBar();
}

void ScalingProfileViewModel::IsReserveTitleBar(bool value) {
	if (_profile->MagSettings().IsReserveTitleBar() == value) {
		return;
	}

	_profile->MagSettings().IsReserveTitleBar(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsReserveTitleBar"));
}

bool ScalingProfileViewModel::IsCroppingEnabled() const noexcept {
	return _profile->IsCroppingEnabled();
}

void ScalingProfileViewModel::IsCroppingEnabled(bool value) {
	if (_profile->IsCroppingEnabled() == value) {
		return;
	}

	_profile->IsCroppingEnabled(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsCroppingEnabled"));
}

double ScalingProfileViewModel::CroppingLeft() const noexcept {
	return _profile->MagSettings().Cropping().Left;
}

void ScalingProfileViewModel::CroppingLeft(double value) {
	Magpie::Runtime::Cropping cropping = _profile->MagSettings().Cropping();
	if (cropping.Left == value) {
		return;
	}

	cropping.Left = std::isnan(value) ? 0 : value;
	_profile->MagSettings().Cropping(cropping);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingLeft"));
}

double ScalingProfileViewModel::CroppingTop() const noexcept {
	return _profile->MagSettings().Cropping().Top;
}

void ScalingProfileViewModel::CroppingTop(double value) {
	Magpie::Runtime::Cropping cropping = _profile->MagSettings().Cropping();
	if (cropping.Top == value) {
		return;
	}

	// 用户已清空数字框则重置为 0
	cropping.Top = std::isnan(value) ? 0 : value;
	_profile->MagSettings().Cropping(cropping);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingTop"));
}

double ScalingProfileViewModel::CroppingRight() const noexcept {
	return _profile->MagSettings().Cropping().Right;
}

void ScalingProfileViewModel::CroppingRight(double value) {
	Magpie::Runtime::Cropping cropping = _profile->MagSettings().Cropping();
	if (cropping.Right == value) {
		return;
	}

	cropping.Right = std::isnan(value) ? 0 : value;
	_profile->MagSettings().Cropping(cropping);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingRight"));
}

double ScalingProfileViewModel::CroppingBottom() const noexcept {
	return _profile->MagSettings().Cropping().Bottom;
}

void ScalingProfileViewModel::CroppingBottom(double value) {
	Magpie::Runtime::Cropping cropping = _profile->MagSettings().Cropping();
	if (cropping.Bottom == value) {
		return;
	}

	cropping.Bottom = std::isnan(value) ? 0 : value;
	_profile->MagSettings().Cropping(cropping);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CroppingBottom"));
}

bool ScalingProfileViewModel::IsAdjustCursorSpeed() const noexcept {
	return _profile->MagSettings().IsAdjustCursorSpeed();
}

void ScalingProfileViewModel::IsAdjustCursorSpeed(bool value) {
	if (_profile->MagSettings().IsAdjustCursorSpeed() == value) {
		return;
	}

	_profile->MagSettings().IsAdjustCursorSpeed(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsAdjustCursorSpeed"));
}

bool ScalingProfileViewModel::IsDrawCursor() const noexcept {
	return _profile->MagSettings().IsDrawCursor();
}

void ScalingProfileViewModel::IsDrawCursor(bool value) {
	if (_profile->MagSettings().IsDrawCursor() == value) {
		return;
	}

	_profile->MagSettings().IsDrawCursor(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDrawCursor"));
}

int32_t ScalingProfileViewModel::CursorScaling() const noexcept {
	return (int32_t)_profile->CursorScaling();
}

void ScalingProfileViewModel::CursorScaling(int32_t value) {
	if (value < 0) {
		return;
	}

	Magpie::App::CursorScaling cursorScaling = (Magpie::App::CursorScaling)value;
	if (_profile->CursorScaling() == cursorScaling) {
		return;
	}

	_profile->CursorScaling(cursorScaling);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CursorScaling"));
}

double ScalingProfileViewModel::CustomCursorScaling() const noexcept {
	return _profile->CustomCursorScaling();
}

void ScalingProfileViewModel::CustomCursorScaling(double value) {
	if (_profile->CustomCursorScaling() == value) {
		return;
	}

	_profile->CustomCursorScaling(std::isnan(value) ? 1.0 : value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CustomCursorScaling"));
}

int32_t ScalingProfileViewModel::CursorInterpolationMode() const noexcept {
	return (int32_t)_profile->MagSettings().CursorInterpolationMode();
}

void ScalingProfileViewModel::CursorInterpolationMode(int32_t value) {
	if (value < 0) {
		return;
	}

	Magpie::Runtime::CursorInterpolationMode cursorInterpolationMode = (Magpie::Runtime::CursorInterpolationMode)value;
	if (_profile->MagSettings().CursorInterpolationMode() == cursorInterpolationMode) {
		return;
	}

	_profile->MagSettings().CursorInterpolationMode(cursorInterpolationMode);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CursorInterpolationMode"));
}

bool ScalingProfileViewModel::IsDisableDirectFlip() const noexcept {
	return _profile->MagSettings().IsDisableDirectFlip();
}

void ScalingProfileViewModel::IsDisableDirectFlip(bool value) {
	if (_profile->MagSettings().IsDisableDirectFlip() == value) {
		return;
	}

	_profile->MagSettings().IsDisableDirectFlip(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDisableDirectFlip"));
}

fire_and_forget ScalingProfileViewModel::_LoadIcon(FrameworkElement const& mainPage) {
	auto weakThis = get_weak();

	bool preferLightTheme = mainPage.ActualTheme() == ElementTheme::Light;
	bool isPackaged = _profile->IsPackaged();
	std::wstring path = _profile->PathRule();
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
