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
using namespace Windows::UI::ViewManagement;


namespace winrt::Magpie::implementation {

MainPage::MainPage() {
	InitializeComponent();

	_hwndHost = (HWND)Application::Current().as<App>().HwndHost();

	_settings = Application::Current().as<Magpie::App>().Settings();

	_UpdateTheme();
	_settings.ThemeChanged([this](const auto&, int) { _UpdateTheme(); });

	Background(Magpie::MicaBrush(*this));
}

MainPage::~MainPage() {
	if (_colorChangedToken) {
		_uiSettings.ColorValuesChanged(_colorChangedToken);
		_colorChangedToken = {};
	}
}

void MainPage::NavigationView_SelectionChanged(
	Microsoft::UI::Xaml::Controls::NavigationView const&,
	Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args
) {
	auto contentFrame = ContentFrame();

	if (args.IsSettingsSelected()) {
		contentFrame.Navigate(winrt::xaml_typename<Magpie::SettingsPage>());
	} else {
		Microsoft::UI::Xaml::Controls::NavigationViewItem selectedItem{ nullptr };
		args.SelectedItem().as(selectedItem);

		hstring tag = unbox_value<hstring>(selectedItem.Tag());
		if (tag == L"Home") {
			contentFrame.Navigate(winrt::xaml_typename<Magpie::HomePage>());
		} else if (tag == L"About") {
			contentFrame.Navigate(winrt::xaml_typename<Magpie::AboutPage>());
		}
	}
}

Microsoft::UI::Xaml::Controls::NavigationView MainPage::RootNavigationView() {
	return __super::RootNavigationView();
}

void MainPage::_UpdateTheme() {
	constexpr const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 = 19;
	constexpr const DWORD DWMWA_MICA_EFFECT = 1029;

	int theme = _settings.Theme();

	BOOL isDarkTheme = FALSE;
	if (theme == 2) {
		if (!_colorChangedToken) {
			_colorChangedToken = _uiSettings.ColorValuesChanged({ this, &MainPage::_Settings_ColorValuesChanged });
		}

		isDarkTheme = _uiSettings.GetColorValue(UIColorType::Background).R < 128;
	} else {
		if (_colorChangedToken) {
			_uiSettings.ColorValuesChanged(_colorChangedToken);
			_colorChangedToken = {};
		}

		isDarkTheme = theme == 1;
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
		DwmSetWindowAttribute(_hwndHost, DWMWA_MICA_EFFECT, &mica, sizeof(mica));
	}

	RequestedTheme(isDarkTheme ? ElementTheme::Dark : ElementTheme::Light);

	// 使标题栏适应黑暗模式
	// build 18985 之前 DWMWA_USE_IMMERSIVE_DARK_MODE 的值不同
	// https://github.com/MicrosoftDocs/sdk-api/pull/966/files
	DwmSetWindowAttribute(
		_hwndHost,
		osBuild < 18985 ? DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 : DWMWA_USE_IMMERSIVE_DARK_MODE,
		& isDarkTheme,
		sizeof(isDarkTheme)
	);

	// 更改背景色以配合主题
	// 背景色在更改窗口大小时会短暂可见
	HBRUSH hbrOld = (HBRUSH)SetClassLongPtr(_hwndHost, GCLP_HBRBACKGROUND,
		(INT_PTR)CreateSolidBrush(isDarkTheme ? RGB(32, 32, 32) : RGB(243, 243, 243)));
	if (hbrOld) {
		DeleteObject(hbrOld);
	}
	InvalidateRect(_hwndHost, nullptr, TRUE);

	// 强制重绘标题栏
	LONG_PTR style = GetWindowLongPtr(_hwndHost, GWL_EXSTYLE);
	if (osBuild < 22000) {
		// 在 Win10 上需要更多 hack
		SetWindowLongPtr(_hwndHost, GWL_EXSTYLE, style | WS_EX_LAYERED);
		SetLayeredWindowAttributes(_hwndHost, 0, 254, LWA_ALPHA);
	}
	SetWindowLongPtr(_hwndHost, GWL_EXSTYLE, style);

	Logger::Get().Info(StrUtils::Concat("当前主题：", isDarkTheme ? "深色" : "浅色"));
}

IAsyncAction MainPage::_Settings_ColorValuesChanged(UISettings const&, IInspectable const&) {
	return Dispatcher().RunAsync(
		CoreDispatcherPriority::Normal,
		{ this, &MainPage::_UpdateTheme }
	);
}

} // namespace winrt::Magpie::implementation
