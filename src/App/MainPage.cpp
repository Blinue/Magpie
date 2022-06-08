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

	// 修复 WinUI 的汉堡菜单的尺寸 bug
	__super::RootNavigationView().IsPaneOpen(true);

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
	int theme = _settings.Theme();

	bool isDarkTheme = FALSE;
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

	if (_isDarkTheme.has_value() && _isDarkTheme == isDarkTheme) {
		// 无需切换
		return;
	}

	_isDarkTheme = isDarkTheme;

	RequestedTheme(isDarkTheme ? ElementTheme::Dark : ElementTheme::Light);
	Utils::UpdateThemeOfXamlPopups(XamlRoot(), ActualTheme());

	Logger::Get().Info(StrUtils::Concat("当前主题：", isDarkTheme ? "深色" : "浅色"));
}

IAsyncAction MainPage::_Settings_ColorValuesChanged(UISettings const&, IInspectable const&) {
	return Dispatcher().RunAsync(
		CoreDispatcherPriority::Normal,
		{ this, &MainPage::_UpdateTheme }
	);
}

} // namespace winrt::Magpie::implementation
