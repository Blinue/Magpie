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
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Controls;


namespace winrt::Magpie::App::implementation {

MainPage::MainPage() {
	InitializeComponent();

	_UpdateTheme();
	AppSettings::Get().ThemeChanged([this](int) { _UpdateTheme(); });

	Background(MicaBrush(*this));

	IVector<IInspectable> navMenuItems = __super::RootNavigationView().MenuItems();
	for (const ScalingProfile& profile : AppSettings::Get().ScalingProfiles()) {
		MUXC::NavigationViewItem item;
		item.Content(box_value(profile.Name()));
		Controls::FontIcon icon;
		icon.Glyph(L"\uECAA");
		item.Icon(icon);
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
		contentFrame.Navigate(winrt::xaml_typename<SettingsPage>());
	} else {
		IInspectable tag = args.SelectedItem().as<MUXC::NavigationViewItem>().Tag();
		if (tag) {
			hstring tagStr = unbox_value<hstring>(tag);
			Interop::TypeName typeName;
			if (tagStr == L"Home") {
				typeName = winrt::xaml_typename<HomePage>();
			} else if (tagStr == L"ScalingModes") {
				typeName = winrt::xaml_typename<ScalingModesPage>();
			} else if (tagStr == L"About") {
				typeName = winrt::xaml_typename<AboutPage>();
			} else {
				typeName = winrt::xaml_typename<HomePage>();
			}

			contentFrame.Navigate(typeName);
		} else {
			// 缩放配置页面
			MUXC::NavigationView nv = __super::RootNavigationView();
			uint32_t index;
			if (nv.MenuItems().IndexOf(nv.SelectedItem(), index)) {
				contentFrame.Navigate(winrt::xaml_typename<ScalingProfilePage>(), box_value(index - 3));
			}
		}
	}
}

IAsyncAction MainPage::AddNavigationViewItem_Tapped(IInspectable const&, TappedRoutedEventArgs const&) {
	if (!_newProfileDialog) {
		// 惰性初始化
		_newProfileDialog = ContentDialog();
		_newProfileDialogContent = NewProfileDialog();

		_newProfileDialog.Title(box_value(L"添加新配置"));
		_newProfileDialog.Content(_newProfileDialogContent);
		_newProfileDialog.PrimaryButtonText(L"确定");
		_newProfileDialog.CloseButtonText(L"取消");
		_newProfileDialog.DefaultButton(ContentDialogButton::Primary);
		_newProfileDialog.Closing({ this, &MainPage::_NewProfileDialog_Closing });
	}

	_newProfileDialog.XamlRoot(XamlRoot());
	_newProfileDialog.RequestedTheme(ActualTheme());

	// 防止快速点击时崩溃
	static bool isShowing = false;
	if (isShowing) {
		co_return;
	}
	isShowing = true;
	co_await _newProfileDialog.ShowAsync();
	isShowing = false;
}

MUXC::NavigationView MainPage::RootNavigationView() {
	return __super::RootNavigationView();
}

void MainPage::_NewProfileDialog_Closing(Controls::ContentDialog const&, Controls::ContentDialogClosingEventArgs const& args) {
	
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
