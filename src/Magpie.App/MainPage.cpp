#include "pch.h"
#include <winrt/Windows.System.Profile.h>
#include "MicaBrush.h"
#include "HomePage.h"
#include "SettingsPage.h"
#include "AboutPage.h"
#include "MainPage.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::Foundation;
using namespace Windows::System::Profile;
using namespace Windows::UI::ViewManagement;
using namespace Microsoft::UI::Xaml::Controls;


static UINT GetOSBuild() {
	const winrt::hstring& deviceFamilyVersion = AnalyticsInfo::VersionInfo().DeviceFamilyVersion();
	uint64_t version = std::stoull(deviceFamilyVersion.c_str());
	return (version & 0x00000000FFFF0000L) >> 16;
}

namespace winrt::Magpie::App::implementation {

MainPage::MainPage() {
	InitializeComponent();
}

MainPage::~MainPage() {
	if (_colorChangedToken) {
		_uiSettings.ColorValuesChanged(_colorChangedToken);
		_colorChangedToken = {};
	}
}

void MainPage::NavigationView_SelectionChanged(NavigationView const&, NavigationViewSelectionChangedEventArgs const& args) {
	if (args.IsSettingsSelected()) {
		ContentFrame().Navigate(winrt::xaml_typename<Magpie::App::SettingsPage>());
	} else {
		NavigationViewItem selectedItem{ nullptr };
		args.SelectedItem().as(selectedItem);

		hstring tag = unbox_value<hstring>(selectedItem.Tag());
		if (tag == L"Home") {
			ContentFrame().Navigate(winrt::xaml_typename<Magpie::App::HomePage>());
		} else if (tag == L"About") {
			ContentFrame().Navigate(winrt::xaml_typename<Magpie::App::AboutPage>());
		}
	}
}

void MainPage::NavigationView_DisplayModeChanged(NavigationView const& sender, NavigationViewDisplayModeChangedEventArgs const& args) {
	ScalingConfigSeparator().Visibility(
		args.DisplayMode() == NavigationViewDisplayMode::Compact ? Visibility::Visible : Visibility::Collapsed);
}

void MainPage::Theme(uint8_t theme) {
	assert(theme >= 0 && theme <= 2);
	_theme = theme;
	_UpdateHostTheme();
}

void MainPage::HostWnd(uint64_t value) {
	_hostWnd = value;

	_micaBrush = Magpie::App::MicaBrush(*this);
	_UpdateHostTheme();

	Background(_micaBrush);
}

void MainPage::OnHostFocusChanged(bool isFocused) {
	_micaBrush.OnHostFocusChanged(isFocused);
}

void MainPage::_UpdateHostTheme() {
	constexpr const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 = 19;
	constexpr const DWORD DWMWA_MICA_EFFECT = 1029;

	HWND hwndHost = (HWND)_hostWnd;
	if (!hwndHost) {
		return;
	}

	BOOL isDarkTheme = FALSE;
	if (_theme == 2) {
		if (!_colorChangedToken) {
			_colorChangedToken = _uiSettings.ColorValuesChanged({ this, &MainPage::_Settings_ColorValuesChanged });
		}

		isDarkTheme = _uiSettings.GetColorValue(UIColorType::Background).R < 128;
	} else {
		if (_colorChangedToken) {
			_uiSettings.ColorValuesChanged(_colorChangedToken);
			_colorChangedToken = {};
		}

		isDarkTheme = _theme == 1;
	}

	if (_isDarkTheme.has_value() && _isDarkTheme == (bool)isDarkTheme) {
		// 无需切换
		return;
	}

	_isDarkTheme = isDarkTheme;

	auto osBuild = GetOSBuild();

	if (osBuild >= 22000) {
		// 在 Win11 中应用 Mica
		BOOL mica = TRUE;
		DwmSetWindowAttribute(hwndHost, DWMWA_MICA_EFFECT, &mica, sizeof(mica));
	}

	RequestedTheme(isDarkTheme ? ElementTheme::Dark : ElementTheme::Light);

	// 使标题栏适应黑暗模式
	// build 18985 之前 DWMWA_USE_IMMERSIVE_DARK_MODE 的值不同
	// https://github.com/MicrosoftDocs/sdk-api/pull/966/files
	DwmSetWindowAttribute(
		hwndHost,
		osBuild < 18985 ? DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 : DWMWA_USE_IMMERSIVE_DARK_MODE,
		& isDarkTheme,
		sizeof(isDarkTheme)
	);

	// 更改背景色以配合主题
	// 背景色在更改窗口大小时会短暂可见
	HBRUSH hbrOld = (HBRUSH)SetClassLongPtr(hwndHost, GCLP_HBRBACKGROUND,
		(INT_PTR)CreateSolidBrush(isDarkTheme ? RGB(0, 0, 0) : RGB(255, 255, 255)));
	DeleteObject(hbrOld);
	InvalidateRect(hwndHost, nullptr, TRUE);

	// 强制重绘标题栏
	auto style = GetWindowLongPtr(hwndHost, GWL_EXSTYLE);
	if (osBuild < 22000) {
		// 在 Win10 上需要更多 hack
		SetWindowLongPtr(hwndHost, GWL_EXSTYLE, style | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hwndHost, 0, 254, LWA_ALPHA);
	}
	SetWindowLongPtr(hwndHost, GWL_EXSTYLE, style);
}

IAsyncAction MainPage::_Settings_ColorValuesChanged(UISettings const&, IInspectable const&) {
	return Dispatcher().RunAsync(
		CoreDispatcherPriority::Normal,
		{ this, &MainPage::_UpdateHostTheme }
	);
}

} // namespace winrt::Magpie::App::implementation
