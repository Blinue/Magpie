#include "pch.h"
#include "MainPage.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif
#include <winrt/Windows.System.Profile.h>
#include "MicaBrush.h"
#include "HomePage.h"
#include "SettingsPage.h"
#include "AboutPage.h"
#include "Utils.h"
#include "Logger.h"
#include "StrUtils.h"


using namespace winrt;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::Foundation;
using namespace Windows::System::Profile;
using namespace Windows::UI::ViewManagement;
using namespace Microsoft::UI::Xaml::Controls;


namespace winrt::Magpie::App::implementation {

MainPage::MainPage() {
	InitializeComponent();

	// 修复 WinUI 的汉堡菜单的尺寸 bug
	GlobalNavigationView().IsPaneOpen(true);
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
	// 需判断 DisplayMode，似乎是 WinUI 的 bug
	ScalingConfigSeparator().Visibility(sender.IsPaneOpen() && (args.DisplayMode() != NavigationViewDisplayMode::Compact)
		? Visibility::Collapsed : Visibility::Visible);
}

void MainPage::NavigationView_PaneOpening(NavigationView const&, IInspectable const&) {
	ScalingConfigSeparator().Visibility(Visibility::Collapsed);
}

void MainPage::NavigationView_PaneClosing(NavigationView const&, NavigationViewPaneClosingEventArgs const&) {
	ScalingConfigSeparator().Visibility(Visibility::Visible);
}

void MainPage::Theme(uint8_t theme) {
	assert(theme >= 0 && theme <= 2);
	_theme = theme;
	_UpdateTheme();
}

void MainPage::Initialize(uint64_t hwndHost, uint64_t pLogger) {
	_hostWnd = hwndHost;

	Logger& logger = Logger::Get();
	logger.Initialize(*(Logger*)pLogger);

	_micaBrush = Magpie::App::MicaBrush(*this);
	_UpdateTheme();

	Background(_micaBrush);
}

void MainPage::OnHostFocusChanged(bool isFocused) {
	_micaBrush.OnHostFocusChanged(isFocused);
}

void MainPage::_UpdateTheme() {
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

	auto osBuild = Utils::GetOSBuild();

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
	LONG_PTR style = GetWindowLongPtr(hwndHost, GWL_EXSTYLE);
	if (osBuild < 22000) {
		// 在 Win10 上需要更多 hack
		SetWindowLongPtr(hwndHost, GWL_EXSTYLE, style | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hwndHost, 0, 254, LWA_ALPHA);
	}
	SetWindowLongPtr(hwndHost, GWL_EXSTYLE, style);

	Logger::Get().Info(StrUtils::Concat("当前主题：", isDarkTheme ? "深色" : "浅色"));
}

IAsyncAction MainPage::_Settings_ColorValuesChanged(UISettings const&, IInspectable const&) {
	return Dispatcher().RunAsync(
		CoreDispatcherPriority::Normal,
		{ this, &MainPage::_UpdateTheme }
	);
}

} // namespace winrt::Magpie::App::implementation
