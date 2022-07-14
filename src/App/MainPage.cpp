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
#include "ScalingProfileService.h"
#include "AppXReader.h"
#include "IconHelper.h"


using namespace winrt;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media::Imaging;


namespace winrt::Magpie::App::implementation {

MainPage::MainPage() {
	InitializeComponent();

	_UpdateTheme();
	AppSettings::Get().ThemeChanged([this](int) { _UpdateTheme(); });

	Background(MicaBrush(*this));

	_displayInfomation = DisplayInformation::GetForCurrentView();
	uint32_t dpi = (uint32_t)std::lroundf(_displayInfomation.LogicalDpi());
	uint32_t preferredIconSize = (uint32_t)std::ceil(dpi * 16 / 96.0);
	bool preferLightTheme = ActualTheme() == ElementTheme::Light;

	IVector<IInspectable> navMenuItems = __super::RootNavigationView().MenuItems();
	for (const ScalingProfile& profile : AppSettings::Get().ScalingProfiles()) {
		MUXC::NavigationViewItem item;
		item.Content(box_value(profile.Name()));

		([](bool isPackaged, std::wstring path, uint32_t preferredIconSize, bool preferLightTheme, MUXC::NavigationViewItem const& item, CoreDispatcher dispatcher)->fire_and_forget {
			weak_ref<MUXC::NavigationViewItem> weakRef(item);

			co_await resume_background();

			std::wstring iconPath;
			bool hasBackground = false;
			SoftwareBitmap iconBitmap{ nullptr };

			if (isPackaged) {
				AppXReader reader;
				reader.Initialize(path);
				iconPath = reader.GetIconPath(preferredIconSize, preferLightTheme, &hasBackground);
			} else {
				iconBitmap = IconHelper::GetIconOfExe(path.c_str(), preferredIconSize);
			}

			co_await dispatcher;
			
			if (auto strongRef = weakRef.get()) {
				if (!iconPath.empty()) {
					BitmapImage image;
					image.UriSource(Uri(iconPath));

					MUXC::ImageIcon imageIcon;
					imageIcon.Source(image);

					if (hasBackground) {
						/*imageIcon.Width(12);
						imageIcon.Height(12);

						StackPanel container;
						container.Background(Application::Current().Resources().Lookup(box_value(L"SystemControlHighlightAccentBrush")).as<SolidColorBrush>());
						container.VerticalAlignment(VerticalAlignment::Center);
						container.HorizontalAlignment(HorizontalAlignment::Center);
						container.Padding({ 2,2,2,2 });
						container.Children().Append(imageIcon);

						_icon = std::move(container);*/
					} else {
						imageIcon.Width(16);
						imageIcon.Height(16);

						strongRef.Icon(imageIcon);
					}
				} else if (iconBitmap) {
					SoftwareBitmapSource imageSource;
					co_await imageSource.SetBitmapAsync(iconBitmap);

					MUXC::ImageIcon imageIcon;
					imageIcon.Width(16);
					imageIcon.Height(16);
					imageIcon.Source(imageSource);

					strongRef.Icon(imageIcon);
				} else {

				}
			}
		})(profile.IsPackaged(), profile.PathRule(), preferredIconSize, preferLightTheme, item, Dispatcher());

		navMenuItems.InsertAt(navMenuItems.Size() - 1, item);
	}

	// Win10 里启动时有一个 ToggleSwitch 的动画 bug，这里展示页面切换动画掩盖
	if (Win32Utils::GetOSBuild() < 22000) {
		ContentFrame().Navigate(winrt::xaml_typename<Controls::Page>());
	}

	_profileAddedRevoker = ScalingProfileService::Get().ProfileAdded(
		auto_revoke, { this, &MainPage::_ScalingProfileService_ProfileAdded });
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

	// 设置 NavigationView 内的 Tooltip 的主题
	XamlUtils::UpdateThemeOfTooltips(*this, ActualTheme());
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

void MainPage::NavigationView_PaneOpening(MUXC::NavigationView const&, IInspectable const&) {
	if (Win32Utils::GetOSBuild() >= 22000) {
		// Win11 中 Tooltip 自动适应主题
		return;
	}

	XamlUtils::UpdateThemeOfTooltips(*this, ActualTheme());

	// UpdateThemeOfTooltips 中使用的 hack 会使 NavigationViewItem 在展开时不会自动删除 Tooltip
	// 因此这里手动删除
	const MUXC::NavigationView& nv = __super::RootNavigationView();
	for (const IInspectable& item : nv.MenuItems()) {
		ToolTipService::SetToolTip(item.as<DependencyObject>(), nullptr);
	}
	for (const IInspectable& item : nv.FooterMenuItems()) {
		ToolTipService::SetToolTip(item.as<DependencyObject>(), nullptr);
	}
}

void MainPage::NavigationView_PaneClosing(MUXC::NavigationView const&, MUXC::NavigationViewPaneClosingEventArgs const&) {
	XamlUtils::UpdateThemeOfTooltips(*this, ActualTheme());
}

void MainPage::NavigationView_DisplayModeChanged(MUXC::NavigationView const&, MUXC::NavigationViewDisplayModeChangedEventArgs const&) {
	XamlUtils::UpdateThemeOfTooltips(*this, ActualTheme());
}

IAsyncAction MainPage::AddNavigationViewItem_Tapped(IInspectable const&, TappedRoutedEventArgs const&) {
	if (!_newProfileDialog) {
		// 惰性初始化
		_newProfileDialog = ContentDialog();
		_newProfileDialog.Title(box_value(L"添加新配置"));
		_newProfileDialog.PrimaryButtonText(L"确定");
		_newProfileDialog.CloseButtonText(L"取消");
		_newProfileDialog.DefaultButton(ContentDialogButton::Primary);
	}

	_newProfileDialog.Content(NewProfileDialog());
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

	if (IsLoaded() && (ActualTheme() == ElementTheme::Dark) == isDarkTheme) {
		// 无需切换
		return;
	}

	ElementTheme newTheme = isDarkTheme ? ElementTheme::Dark : ElementTheme::Light;

	RequestedTheme(newTheme);
	XamlUtils::UpdateThemeOfXamlPopups(XamlRoot(), newTheme);
	XamlUtils::UpdateThemeOfTooltips(*this, newTheme);

	Logger::Get().Info(StrUtils::Concat("当前主题：", isDarkTheme ? "深色" : "浅色"));
}

IAsyncAction MainPage::_Settings_ColorValuesChanged(UISettings const&, IInspectable const&) {
	return Dispatcher().RunAsync(
		CoreDispatcherPriority::Normal,
		{ this, &MainPage::_UpdateTheme }
	);
}

void MainPage::_ScalingProfileService_ProfileAdded(ScalingProfile& profile) {
	MUXC::NavigationViewItem item;
	item.Content(box_value(profile.Name()));
	Controls::FontIcon icon;
	icon.Glyph(L"\uECAA");
	item.Icon(icon);

	IVector<IInspectable> navMenuItems = __super::RootNavigationView().MenuItems();
	navMenuItems.InsertAt(navMenuItems.Size() - 1, item);
	
	item.IsSelected(true);
}

} // namespace winrt::Magpie::implementation
