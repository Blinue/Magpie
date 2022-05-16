#include "pch.h"
#include <winrt/Windows.System.Profile.h>
#include "MicaBrush.h"
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

void MainPage::ThemeRadioButton_Checked(IInspectable const& sender, RoutedEventArgs const&) {
	if (sender == LightThemeRadioButton()) {
		_theme = 0;
	} else if (sender == DarkThemeRadioButton()) {
		_theme = 1;
	} else {
		_theme = 2;
	}

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

	BOOL isDarkMode = FALSE;
	if (_theme == 0) {
		isDarkMode = FALSE;
	} else if (_theme == 1) {
		isDarkMode = TRUE;
	} else {
		isDarkMode = _uiSettings.GetColorValue(UIColorType::Background).R < 128;
	}

	if (_isDarkTheme.has_value() && _isDarkTheme == (bool)isDarkMode) {
		// 无需切换
		return;
	}

	_isDarkTheme = isDarkMode;

	if (_theme == 2) {
		if (!_colorChangedToken) {
			_colorChangedToken = _uiSettings.ColorValuesChanged({ this, &MainPage::_Settings_ColorValuesChanged });
		}
	} else {
		if (_colorChangedToken) {
			_uiSettings.ColorValuesChanged(_colorChangedToken);
			_colorChangedToken = {};
		}
	}

	auto osBuild = GetOSBuild();

	// 使标题栏适应黑暗模式
	// build 18985 之前 DWMWA_USE_IMMERSIVE_DARK_MODE 的值不同
	// https://github.com/MicrosoftDocs/sdk-api/pull/966/files
	DwmSetWindowAttribute(
		hwndHost,
		osBuild < 18985 ? DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 : DWMWA_USE_IMMERSIVE_DARK_MODE,
		&isDarkMode,
		sizeof(isDarkMode)
	);

	if (osBuild >= 22000) {
		// 在 Win11 中应用 Mica
		BOOL mica = TRUE;
		DwmSetWindowAttribute(hwndHost, DWMWA_MICA_EFFECT, &mica, sizeof(mica));
	}

	// 更改背景色以配合主题
	// 背景色在更改窗口大小时会短暂可见
	HBRUSH hbrOld = (HBRUSH)SetClassLongPtr(hwndHost, GCLP_HBRBACKGROUND,
		(INT_PTR)CreateSolidBrush(isDarkMode ? RGB(0, 0, 0) : RGB(255, 255, 255)));
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

	RequestedTheme(isDarkMode ? ElementTheme::Dark : ElementTheme::Light);
}

IAsyncAction MainPage::_Settings_ColorValuesChanged(UISettings const&, IInspectable const&) {
	return Dispatcher().RunAsync(
		CoreDispatcherPriority::Normal,
		{ this, &MainPage::_UpdateHostTheme }
	);
}

} // namespace winrt::Magpie::App::implementation
