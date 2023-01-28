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
#include "ComboBoxHelper.h"
#include "CommonSharedConstants.h"
#include "ContentDialogHelper.h"

using namespace winrt;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media::Imaging;

namespace winrt::Magpie::App::implementation {

static constexpr const uint32_t FIRST_PROFILE_ITEM_IDX = 4;

MainPage::MainPage() {
	InitializeComponent();

	_themeChangedRevoker = AppSettings::Get().ThemeChanged(auto_revoke, [this](int) { _UpdateTheme(); });
	_UpdateTheme(false);

	_colorValuesChangedRevoker = _uiSettings.ColorValuesChanged(
		auto_revoke, { get_weak(), &MainPage::_UISettings_ColorValuesChanged });

	const Win32Utils::OSVersion& osVersion = Win32Utils::GetOSVersion();
	if (osVersion.Is22H2OrNewer()) {
		// Win11 22H2+ 使用系统的 Mica 背景
		MUXC::BackdropMaterial::SetApplyToRootOrPageBackground(*this, true);
	}
	
	_displayInformation = DisplayInformation::GetForCurrentView();
	_dpiChangedRevoker = _displayInformation.DpiChanged(
		auto_revoke, [this](DisplayInformation const&, IInspectable const&) { _UpdateIcons(false); });

	IVector<IInspectable> navMenuItems = __super::RootNavigationView().MenuItems();
	for (const ScalingProfile& profile : AppSettings::Get().ScalingProfiles()) {
		MUXC::NavigationViewItem item;
		item.Content(box_value(profile.name));
		// 用于占位
		item.Icon(FontIcon());
		_LoadIcon(item, profile);

		navMenuItems.InsertAt(navMenuItems.Size() - 1, item);
	}

	// Win10 里启动时有一个 ToggleSwitch 的动画 bug，这里展示页面切换动画掩盖
	if (!osVersion.IsWin11()) {
		ContentFrame().Navigate(winrt::xaml_typename<Controls::Page>());
	}

	ScalingProfileService& scalingProfileService = ScalingProfileService::Get();
	_profileAddedRevoker = scalingProfileService.ProfileAdded(
		auto_revoke, { this, &MainPage::_ScalingProfileService_ProfileAdded });
	_profileRenamedRevoker = scalingProfileService.ProfileRenamed(
		auto_revoke, { this, &MainPage::_ScalingProfileService_ProfileRenamed });
	_profileRemovedRevoker = scalingProfileService.ProfileRemoved(
		auto_revoke, { this, &MainPage::_ScalingProfileService_ProfileRemoved });
	_profileMovedRevoker = scalingProfileService.ProfileMoved(
		auto_revoke, { this, &MainPage::_ScalingProfileService_ProfileReordered });
}

MainPage::~MainPage() {
	ContentDialogHelper::CloseActiveDialog();

	// 不手动置空会内存泄露
	// 似乎是 XAML Islands 的 bug？
	ContentFrame().Content(nullptr);

	// 每次主窗口关闭都清理 AppXReader 的缓存
	AppXReader::ClearCache();
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
		IInspectable selectedItem = args.SelectedItem();
		if (!selectedItem) {
			contentFrame.Content(nullptr);
			return;
		}

		IInspectable tag = selectedItem.as<MUXC::NavigationViewItem>().Tag();
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
				contentFrame.Navigate(winrt::xaml_typename<ScalingProfilePage>(), box_value((int)index - 4));
			}
		}
	}
}

void MainPage::NavigationView_PaneOpening(MUXC::NavigationView const&, IInspectable const&) {
	if (Win32Utils::GetOSVersion().IsWin11()) {
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

fire_and_forget MainPage::NavigationView_ItemInvoked(MUXC::NavigationView const&, MUXC::NavigationViewItemInvokedEventArgs const& args) {
	if (args.InvokedItemContainer() == NewProfileNavigationViewItem()) {
		const UINT dpi = (UINT)std::lroundf(_displayInformation.LogicalDpi());
		const bool isLightTheme = ActualTheme() == ElementTheme::Light;
		_newProfileViewModel.PrepareForOpen(dpi, isLightTheme, Dispatcher());

		// 同步调用 ShowAt 有时会失败
		co_await Dispatcher().TryRunAsync(CoreDispatcherPriority::Normal, [this]() {
			// 仅限 Win10：导航栏处于 Minimal 状态时会导致 Flyout 不在正确位置弹出
			// 有一个修复方法，但会导致性能损失
			NewProfileFlyout().ShowAt(NewProfileNavigationViewItem());
		});
	}
}

MUXC::NavigationView MainPage::RootNavigationView() {
	return __super::RootNavigationView();
}

void MainPage::ComboBox_DropDownOpened(IInspectable const&, IInspectable const&) {
	XamlUtils::UpdateThemeOfXamlPopups(XamlRoot(), ActualTheme());
}

void MainPage::NewProfileConfirmButton_Click(IInspectable const&, RoutedEventArgs const&) {
	_newProfileViewModel.Confirm();
	NewProfileFlyout().Hide();
}

void MainPage::NewProfileNameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args) {
	if (args.Key() == VirtualKey::Enter && _newProfileViewModel.IsConfirmButtonEnabled()) {
		NewProfileConfirmButton_Click(nullptr, nullptr);
	}
}

static Color Win32ColorToWinRTColor(COLORREF color) {
	return { 255, GetRValue(color), GetGValue(color), GetBValue(color) };
}

// 来自 https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/apply-windows-themes#know-when-dark-mode-is-enabled
static bool IsColorLight(const Color& clr) {
	return 5 * clr.G + 2 * clr.R + clr.B > 8 * 128;
}

void MainPage::_UpdateTheme(bool updateIcons) {
	int theme = AppSettings::Get().Theme();

	bool isDarkTheme = FALSE;
	if (theme == 2) {
		// 前景色是亮色表示当前是深色主题
		isDarkTheme = IsColorLight(_uiSettings.GetColorValue(UIColorType::Foreground));
	} else {
		isDarkTheme = theme == 1;
	}

	if (IsLoaded() && (ActualTheme() == ElementTheme::Dark) == isDarkTheme) {
		// 无需切换
		return;
	}

	if (!Win32Utils::GetOSVersion().Is22H2OrNewer()) {
		const Windows::UI::Color bkgColor = Win32ColorToWinRTColor(
			isDarkTheme ? CommonSharedConstants::DARK_TINT_COLOR : CommonSharedConstants::LIGHT_TINT_COLOR);
		Background(SolidColorBrush(bkgColor));
	}

	ElementTheme newTheme = isDarkTheme ? ElementTheme::Dark : ElementTheme::Light;
	RequestedTheme(newTheme);

	XamlUtils::UpdateThemeOfXamlPopups(XamlRoot(), newTheme);
	XamlUtils::UpdateThemeOfTooltips(*this, newTheme);
	NewProfileAdminToolTip().RequestedTheme(newTheme);

	if (updateIcons && IsLoaded()) {
		_UpdateIcons(true);
	}
}

fire_and_forget MainPage::_LoadIcon(MUXC::NavigationViewItem const& item, const ScalingProfile& profile) {
	weak_ref<MUXC::NavigationViewItem> weakRef(item);

	bool preferLightTheme = ActualTheme() == ElementTheme::Light;
	bool isPackaged = profile.isPackaged;
	std::wstring path = profile.pathRule;
	CoreDispatcher dispatcher = Dispatcher();
	uint32_t dpi = (uint32_t)std::lroundf(_displayInformation.LogicalDpi());

	co_await resume_background();

	std::wstring iconPath;
	SoftwareBitmap iconBitmap{ nullptr };

	if (isPackaged) {
		AppXReader reader;
		if (reader.Initialize(path)) {
			std::variant<std::wstring, SoftwareBitmap> uwpIcon =
				reader.GetIcon((uint32_t)std::ceil(dpi * 16 / 96.0), preferLightTheme);
			if (uwpIcon.index() == 0) {
				iconPath = std::get<0>(uwpIcon);
			} else {
				iconBitmap = std::get<1>(uwpIcon);
			}
		}
	} else {
		iconBitmap = IconHelper::ExtractIconFromExe(path.c_str(), 16, dpi);
	}

	co_await dispatcher;

	auto strongRef = weakRef.get();
	if (!strongRef) {
		co_return;
	}

	if (!iconPath.empty()) {
		BitmapIcon icon;
		icon.ShowAsMonochrome(false);
		icon.UriSource(Uri(iconPath));
		icon.Width(16);
		icon.Height(16);

		strongRef.Icon(icon);
	} else if (iconBitmap) {
		SoftwareBitmapSource imageSource;
		co_await imageSource.SetBitmapAsync(iconBitmap);

		MUXC::ImageIcon imageIcon;
		imageIcon.Width(16);
		imageIcon.Height(16);
		imageIcon.Source(imageSource);

		strongRef.Icon(imageIcon);
	} else {
		FontIcon icon;
		icon.Glyph(L"\uECAA");
		strongRef.Icon(icon);
	}
}

fire_and_forget MainPage::_UISettings_ColorValuesChanged(Windows::UI::ViewManagement::UISettings const&, IInspectable const&) {
	co_await Dispatcher();

	if (AppSettings::Get().Theme() == 2) {
		_UpdateTheme(false);
	}

	_UpdateIcons(true);
}

void MainPage::_UpdateIcons(bool skipDesktop) {
	IVector<IInspectable> navMenuItems = RootNavigationView().MenuItems();
	const std::vector<ScalingProfile>& profiles = AppSettings::Get().ScalingProfiles();

	for (uint32_t i = 0; i < profiles.size(); ++i) {
		if (skipDesktop && !profiles[i].isPackaged) {
			continue;
		}

		MUXC::NavigationViewItem item = navMenuItems.GetAt(FIRST_PROFILE_ITEM_IDX + i).as<MUXC::NavigationViewItem>();
		_LoadIcon(item, profiles[i]);
	}
}

void MainPage::_ScalingProfileService_ProfileAdded(ScalingProfile& profile) {
	MUXC::NavigationViewItem item;
	item.Content(box_value(profile.name));
	// 用于占位
	item.Icon(FontIcon());
	_LoadIcon(item, profile);

	IVector<IInspectable> navMenuItems = __super::RootNavigationView().MenuItems();
	navMenuItems.InsertAt(navMenuItems.Size() - 1, item);
	RootNavigationView().SelectedItem(item);
}

void MainPage::_ScalingProfileService_ProfileRenamed(uint32_t idx) {
	RootNavigationView().MenuItems()
		.GetAt(FIRST_PROFILE_ITEM_IDX + idx)
		.as<MUXC::NavigationViewItem>()
		.Content(box_value(AppSettings::Get().ScalingProfiles()[idx].name));
}

void MainPage::_ScalingProfileService_ProfileRemoved(uint32_t idx) {
	MUXC::NavigationView nv = RootNavigationView();
	IVector<IInspectable> menuItems = nv.MenuItems();
	nv.SelectedItem(menuItems.GetAt(FIRST_PROFILE_ITEM_IDX - 1));
	menuItems.RemoveAt(FIRST_PROFILE_ITEM_IDX + idx);
}

void MainPage::_ScalingProfileService_ProfileReordered(uint32_t profileIdx, bool isMoveUp) {
	IVector<IInspectable> menuItems = RootNavigationView().MenuItems();

	uint32_t curIdx = FIRST_PROFILE_ITEM_IDX + profileIdx;
	uint32_t otherIdx = isMoveUp ? curIdx - 1 : curIdx + 1;
	
	IInspectable otherItem = menuItems.GetAt(otherIdx);
	menuItems.RemoveAt(otherIdx);
	menuItems.InsertAt(curIdx, otherItem);
}

}
