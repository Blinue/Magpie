#include "pch.h"
#include "MainPage.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif

#include "XamlUtils.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include "AppSettings.h"


using namespace winrt;
using namespace Windows::UI::ViewManagement;


namespace winrt::Magpie::App::implementation {

MainPage::MainPage() {
	InitializeComponent();

	_UpdateTheme();
	AppSettings::Get().ThemeChanged([this](int) { _UpdateTheme(); });

	Background(Magpie::App::MicaBrush(*this));

	IVector<IInspectable> navMenuItems = __super::RootNavigationView().MenuItems();
	for (const ScalingProfile& profile : AppSettings::Get().ScalingProfiles()) {
		MUXC::NavigationViewItem item;
		item.Content(box_value(profile.Name()));
		navMenuItems.InsertAt(navMenuItems.Size() - 1, item);
	}

	// Win10 里启动时有一个 ToggleSwitch 的动画 bug，这里展示页面切换动画掩盖
	if (Win32Utils::GetOSBuild() < 22000) {
		ContentFrame().Navigate(winrt::xaml_typename<Controls::Page>());
	}
}

MainPage::~MainPage() {
	if (_colorChangedToken) {
		_uiSettings.ColorValuesChanged(_colorChangedToken);
		_colorChangedToken = {};
	}
}

void MainPage::Loaded(IInspectable const&, RoutedEventArgs const&) {
	MUXC::NavigationView nv = __super::RootNavigationView();

	if (nv.DisplayMode() == MUXC::NavigationViewDisplayMode::Minimal) {
		nv.IsPaneOpen(true);
	}

	// 修复 WinUI 的汉堡菜单的尺寸 bug
	nv.PaneDisplayMode(MUXC::NavigationViewPaneDisplayMode::Auto);

	// 消除焦点框
	IsTabStop(true);
	Focus(FocusState::Programmatic);
	IsTabStop(false);
}

void MainPage::NavigationView_SelectionChanged(
	MUXC::NavigationView const&,
	MUXC::NavigationViewSelectionChangedEventArgs const& args
) {
	auto contentFrame = ContentFrame();

	if (args.IsSettingsSelected()) {
		contentFrame.Navigate(winrt::xaml_typename<Magpie::App::SettingsPage>());
	} else {
		Microsoft::UI::Xaml::Controls::NavigationViewItem selectedItem{ nullptr };
		args.SelectedItem().as(selectedItem);

		hstring tag = unbox_value<hstring>(selectedItem.Tag());
		Interop::TypeName typeName;
		if (tag == L"Home") {
			typeName = winrt::xaml_typename<Magpie::App::HomePage>();
		} else if (tag == L"ScalingModes") {
			typeName = winrt::xaml_typename<Magpie::App::ScalingModesPage>();
		} else if (tag == L"About") {
			typeName = winrt::xaml_typename<Magpie::App::AboutPage>();
		} else {
			typeName = winrt::xaml_typename<Magpie::App::ScalingProfilePage>();
		}

		if (!typeName.Name.empty()) {
			contentFrame.Navigate(typeName);
		}
	}
}

MUXC::NavigationView MainPage::RootNavigationView() {
	return __super::RootNavigationView();
}

void MainPage::_UpdateTheme() {
	int theme = AppSettings::Get().Theme();

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
	XamlUtils::UpdateThemeOfXamlPopups(XamlRoot(), ActualTheme());

	Logger::Get().Info(StrUtils::Concat("当前主题：", isDarkTheme ? "深色" : "浅色"));
}

IAsyncAction MainPage::_Settings_ColorValuesChanged(UISettings const&, IInspectable const&) {
	return Dispatcher().RunAsync(
		CoreDispatcherPriority::Normal,
		{ this, &MainPage::_UpdateTheme }
	);
}

} // namespace winrt::Magpie::implementation
