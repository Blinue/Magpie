#include "pch.h"
#include "RootPage.h"
#if __has_include("RootPage.g.cpp")
#include "RootPage.g.cpp"
#endif
#include "XamlHelper.h"
#include "Win32Helper.h"
#include "ProfileService.h"
#include "AppXReader.h"
#include "IconHelper.h"
#include "ControlHelper.h"
#include "ThemeHelper.h"
#include "ContentDialogHelper.h"
#include "LocalizationService.h"
#include "App.h"
#include "TitleBarControl.h"
#include "MainWindow.h"
#include "CandidateWindowItem.h"
#include "CommonSharedConstants.h"

using namespace ::Magpie;
using namespace winrt;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media::Animation;
using namespace Windows::UI::Xaml::Media::Imaging;

namespace winrt::Magpie::implementation {

static constexpr uint32_t FIRST_PROFILE_ITEM_IDX = 4;

RootPage::RootPage() {
	// 设置 Language 属性帮助 XAML 选择合适的字体，比如繁体中文使用 Microsoft JhengHei UI，
	// 日语使用 Yu Gothic UI
	Language(LocalizationService::Get().Language());
}

RootPage::~RootPage() {
	ContentDialogHelper::CloseActiveDialog();

	// 不手动置空会内存泄露
	// 似乎是 XAML Islands 的 bug？
	ContentFrame().Content(nullptr);

	// 每次主窗口关闭都清理 AppXReader 的缓存
	AppXReader::ClearCache();
}

void RootPage::InitializeComponent() {
	RootPageT::InitializeComponent();

	_appThemeChangedRevoker = App::Get().ThemeChanged(
		auto_revoke, [this](bool) { _UpdateTheme(true); });
	_UpdateTheme(false);

	_dpiChangedRevoker = App::Get().MainWindow().DpiChanged(
		auto_revoke, [this](uint32_t) { _UpdateIcons(false); });

	ProfileService& profileService = ProfileService::Get();
	_profileAddedRevoker = profileService.ProfileAdded(
		auto_revoke, std::bind_front(&RootPage::_ProfileService_ProfileAdded, this));
	_profileRenamedRevoker = profileService.ProfileRenamed(
		auto_revoke, std::bind_front(&RootPage::_ProfileService_ProfileRenamed, this));
	_profileRemovedRevoker = profileService.ProfileRemoved(
		auto_revoke, std::bind_front(&RootPage::_ProfileService_ProfileRemoved, this));
	_profileMovedRevoker = profileService.ProfileMoved(
		auto_revoke, std::bind_front(&RootPage::_ProfileService_ProfileReordered, this));

	const Win32Helper::OSVersion& osVersion = Win32Helper::GetOSVersion();
	if (osVersion.Is22H2OrNewer()) {
		// Win11 22H2+ 使用系统的 Mica 背景
		MUXC::BackdropMaterial::SetApplyToRootOrPageBackground(*this, true);
	}

	IVector<IInspectable> navMenuItems = RootNavigationView().MenuItems();
	for (const Profile& profile : AppSettings::Get().Profiles()) {
		MUXC::NavigationViewItem item;
		item.Content(box_value(profile.name));
		// 用于占位
		item.Icon(FontIcon());
		_LoadIcon(item, profile);

		navMenuItems.InsertAt(navMenuItems.Size() - 1, item);
	}
}

static void SkipToggleSwitchAnimations(const DependencyObject& elem) {
	FrameworkElement rootGrid = VisualTreeHelper::GetChild(elem, 0).try_as<FrameworkElement>();

	for (VisualStateGroup group : VisualStateManager::GetVisualStateGroups(rootGrid)) {
		for (VisualState state : group.States()) {
			if (Storyboard storyboard = state.Storyboard()) {
				storyboard.SkipToFill();
			}
		}
	}
}

void RootPage::RootPage_Loaded(IInspectable const&, RoutedEventArgs const&) {
	// 消除焦点框
	IsTabStop(true);
	Focus(FocusState::Programmatic);
	IsTabStop(false);

	// 设置 NavigationView 内的 Tooltip 的主题
	XamlHelper::UpdateThemeOfTooltips(RootNavigationView(), ActualTheme());

	// 启动时跳过 ToggleSwitch 的动画
	std::vector<DependencyObject> elems{ *this };
	do {
		std::vector<DependencyObject> temp;

		for (const DependencyObject& elem : elems) {
			const int count = VisualTreeHelper::GetChildrenCount(elem);
			for (int i = 0; i < count; ++i) {
				DependencyObject current = VisualTreeHelper::GetChild(elem, i);

				if (get_class_name(current) == name_of<ToggleSwitch>()) {
					SkipToggleSwitchAnimations(current);
				} else {
					temp.emplace_back(std::move(current));
				}
			}
		}

		elems = std::move(temp);
	} while (!elems.empty());
}

void RootPage::NavigationView_SelectionChanged(
	MUXC::NavigationView const&,
	MUXC::NavigationViewSelectionChangedEventArgs const& args
) {
	auto contentFrame = ContentFrame();

	if (args.IsSettingsSelected()) {
		contentFrame.Navigate(xaml_typename<SettingsPage>());
	} else {
		IInspectable selectedItem = args.SelectedItem();
		if (!selectedItem) {
			contentFrame.Content(nullptr);
			return;
		}

		IInspectable tag = selectedItem.try_as<MUXC::NavigationViewItem>().Tag();
		if (tag) {
			hstring tagStr = unbox_value<hstring>(tag);
			Interop::TypeName typeName;
			if (tagStr == L"Home") {
				typeName = xaml_typename<HomePage>();
			} else if (tagStr == L"ScalingModes") {
				typeName = xaml_typename<ScalingModesPage>();
			} else if (tagStr == L"About") {
				typeName = xaml_typename<AboutPage>();
			} else {
				typeName = xaml_typename<HomePage>();
			}

			contentFrame.Navigate(typeName);
		} else {
			// 缩放配置页面
			MUXC::NavigationView nv = RootNavigationView();
			uint32_t index;
			if (nv.MenuItems().IndexOf(nv.SelectedItem(), index)) {
				contentFrame.Navigate(xaml_typename<ProfilePage>(), box_value((int)index - 4));
			}
		}
	}
}

void RootPage::NavigationView_PaneOpening(MUXC::NavigationView const&, IInspectable const&) {
	if (Win32Helper::GetOSVersion().IsWin11()) {
		// Win11 中 Tooltip 自动适应主题
		return;
	}

	XamlHelper::UpdateThemeOfTooltips(*this, ActualTheme());

	// UpdateThemeOfTooltips 中使用的 hack 会使 NavigationViewItem 在展开时不会自动删除 Tooltip
	// 因此这里手动删除
	const MUXC::NavigationView& nv = RootNavigationView();
	for (const IInspectable& item : nv.MenuItems()) {
		ToolTipService::SetToolTip(item.try_as<DependencyObject>(), nullptr);
	}
	for (const IInspectable& item : nv.FooterMenuItems()) {
		ToolTipService::SetToolTip(item.try_as<DependencyObject>(), nullptr);
	}
}

void RootPage::NavigationView_PaneClosing(MUXC::NavigationView const&, MUXC::NavigationViewPaneClosingEventArgs const&) {
	XamlHelper::UpdateThemeOfTooltips(*this, ActualTheme());
}

void RootPage::NavigationView_DisplayModeChanged(MUXC::NavigationView const& nv, MUXC::NavigationViewDisplayModeChangedEventArgs const&) {
	bool isExpanded = nv.DisplayMode() == MUXC::NavigationViewDisplayMode::Expanded;
	nv.IsPaneToggleButtonVisible(!isExpanded);
	if (isExpanded) {
		// 延迟设置 IsPaneOpen 才能起作用
		Dispatcher().RunAsync(CoreDispatcherPriority::Low, [nv(MUXC::NavigationView(nv))]() {
			nv.IsPaneOpen(true);
		});
	}

	// !!! HACK !!!
	// 使导航栏的可滚动区域不会覆盖标题栏
	FrameworkElement menuItemsScrollViewer = nv.try_as<IControlProtected>()
		.GetTemplateChild(L"MenuItemsScrollViewer").try_as<FrameworkElement>();
	menuItemsScrollViewer.Margin({ 0,isExpanded ? TitleBar().ActualHeight() : 0.0,0,0});

	XamlHelper::UpdateThemeOfTooltips(*this, ActualTheme());
}

void RootPage::NavigationView_ItemInvoked(MUXC::NavigationView const&, MUXC::NavigationViewItemInvokedEventArgs const& args) {
	if (args.InvokedItemContainer() == NewProfileNavigationViewItem()) {
		_newProfileViewModel->PrepareForOpen();

		// 同步调用 ShowAt 有时会失败
		App::Get().Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [that(get_strong())]() {
			that->NewProfileFlyout().ShowAt(that->NewProfileNavigationViewItem());
		});
	}
}

void RootPage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) const {
	ControlHelper::ComboBox_DropDownOpened(sender);
}

void RootPage::NewProfileConfirmButton_Click(IInspectable const&, RoutedEventArgs const&) {
	_newProfileViewModel->Confirm();
	NewProfileFlyout().Hide();
}

void RootPage::NewProfileNameContextFlyout_Opening(IInspectable const&, IInspectable const&) {
	auto menuItems = NewProfileNameContextFlyout().Items();
	
	int idx = _newProfileViewModel->CandidateWindowIndex();
	if (idx < 0) {
		// 隐藏所有选项
		for (const MenuFlyoutItemBase& item : menuItems) {
			if (IInspectable tag = item.Tag(); tag && tag.try_as<int>()) {
				item.Visibility(Visibility::Collapsed);
			}
		}

		return;
	}

	CandidateWindowItem* selectedItem = get_self<CandidateWindowItem>(
		_newProfileViewModel->CandidateWindows().GetAt(idx).try_as<winrt::Magpie::CandidateWindowItem>());

	// 设置每个选项的可见性
	bool shouldInit = true;
	for (const MenuFlyoutItemBase& item : menuItems) {
		IInspectable tag = item.Tag();
		if (!tag) {
			continue;
		}

		std::optional<int> id = tag.try_as<int>();
		if (!id) {
			continue;
		}

		shouldInit = false;

		if (*id == 1) {
			// 填入进程名选项
			item.Visibility(selectedItem->AUMID().empty() ? Visibility::Visible : Visibility::Collapsed);
		} else if (*id == 2) {
			// 填入应用名选项
			item.Visibility(selectedItem->AUMID().empty() ? Visibility::Collapsed : Visibility::Visible);
		} else {
			// 填入窗口标题选项
			item.Visibility(Visibility::Visible);
		}
	}

	if (!shouldInit) {
		return;
	}

	// 惰性初始化
	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);

	// 填入进程名
	MenuFlyoutItem item1;
	FontIcon icon1;
	icon1.Glyph(L"\xE9F5");
	item1.Text(resourceLoader.GetString(L"Root_NewProfileFlyout_NameContextFlyout_ProcessName"));
	item1.Icon(icon1);
	RoutedEventHandler clickHandler([this](IInspectable const&, IInspectable const&) {
		_UpdateNewProfileNameTextBox(false);
	});
	item1.Click(clickHandler);
	item1.Tag(box_value(1));
	menuItems.Append(item1);

	// 填入应用名
	MenuFlyoutItem item2;
	FontIcon icon2;
	icon2.Glyph(L"\xECAA");
	item2.Text(resourceLoader.GetString(L"Root_NewProfileFlyout_NameContextFlyout_AppName"));
	item2.Icon(icon2);
	item2.Click(clickHandler);
	item2.Tag(box_value(2));
	menuItems.Append(item2);

	if (selectedItem->AUMID().empty()) {
		item2.Visibility(Visibility::Collapsed);
	} else {
		item1.Visibility(Visibility::Collapsed);
	}

	// 填入窗口标题
	MenuFlyoutItem item3;
	FontIcon icon3;
	icon3.Glyph(L"\xE737");
	item3.Icon(icon3);
	item3.Text(resourceLoader.GetString(L"Root_NewProfileFlyout_NameContextFlyout_WindowTitle"));
	item3.Click([this](IInspectable const&, IInspectable const&) {
		_UpdateNewProfileNameTextBox(true);
	});
	item3.Tag(box_value(3));
	menuItems.Append(item3);
}

void RootPage::NewProfileNameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args) {
	if (args.Key() == VirtualKey::Enter && _newProfileViewModel->IsConfirmButtonEnabled()) {
		NewProfileConfirmButton_Click(nullptr, nullptr);
	}
}

void RootPage::NavigateToAboutPage() {
	MUXC::NavigationView nv = RootNavigationView();
	nv.SelectedItem(nv.FooterMenuItems().GetAt(0));
}

TitleBarControl& RootPage::TitleBar() {
	return *get_self<TitleBarControl>(RootPageT::TitleBar());
}

static Color Win32ColorToWinRTColor(COLORREF color) {
	return { 255, GetRValue(color), GetGValue(color), GetBValue(color) };
}

void RootPage::_UpdateTheme(bool updateIcons) {
	const bool isLightTheme = App::Get().IsLightTheme();

	if (IsLoaded() && (ActualTheme() == ElementTheme::Light) == isLightTheme) {
		// 无需切换
		return;
	}

	if (!Win32Helper::GetOSVersion().Is22H2OrNewer()) {
		const Windows::UI::Color bkgColor = Win32ColorToWinRTColor(
			isLightTheme ? ThemeHelper::LIGHT_TINT_COLOR : ThemeHelper::DARK_TINT_COLOR);
		Background(SolidColorBrush(bkgColor));
	}

	ElementTheme newTheme = isLightTheme ? ElementTheme::Light : ElementTheme::Dark;
	RequestedTheme(newTheme);

	XamlHelper::UpdateThemeOfXamlPopups(XamlRoot(), newTheme);
	XamlHelper::UpdateThemeOfTooltips(*this, newTheme);

	if (updateIcons && IsLoaded()) {
		_UpdateIcons(true);
	}
}

fire_and_forget RootPage::_LoadIcon(MUXC::NavigationViewItem const& item, const Profile& profile) {
	weak_ref<MUXC::NavigationViewItem> weakRef(item);

	bool preferLightTheme = App::Get().IsLightTheme();
	bool isPackaged = profile.isPackaged;
	std::wstring path = profile.pathRule;
	const uint32_t iconSize = (uint32_t)std::lroundf(
		16.0f * App::Get().MainWindow().CurrentDpi() / USER_DEFAULT_SCREEN_DPI);

	co_await resume_background();

	std::wstring iconPath;
	SoftwareBitmap iconBitmap{ nullptr };

	if (isPackaged) {
		AppXReader reader;
		if (reader.Initialize(path)) {
			std::variant<std::wstring, SoftwareBitmap> uwpIcon =
				reader.GetIcon(iconSize, preferLightTheme);
			if (uwpIcon.index() == 0) {
				iconPath = std::get<0>(uwpIcon);
			} else {
				iconBitmap = std::get<1>(uwpIcon);
			}
		}
	} else {
		iconBitmap = IconHelper::ExtractIconFromExe(path.c_str(), iconSize);
	}

	co_await App::Get().Dispatcher();

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

void RootPage::_UpdateIcons(bool skipDesktop) {
	IVector<IInspectable> navMenuItems = RootNavigationView().MenuItems();
	const std::vector<Profile>& profiles = AppSettings::Get().Profiles();

	for (uint32_t i = 0; i < profiles.size(); ++i) {
		if (skipDesktop && !profiles[i].isPackaged) {
			continue;
		}

		MUXC::NavigationViewItem item = navMenuItems.GetAt(FIRST_PROFILE_ITEM_IDX + i)
			.try_as<MUXC::NavigationViewItem>();
		_LoadIcon(item, profiles[i]);
	}
}

void RootPage::_ProfileService_ProfileAdded(Profile& profile) {
	MUXC::NavigationViewItem item;
	item.Content(box_value(profile.name));
	// 用于占位
	item.Icon(FontIcon());
	_LoadIcon(item, profile);

	IVector<IInspectable> navMenuItems = RootNavigationView().MenuItems();
	navMenuItems.InsertAt(navMenuItems.Size() - 1, item);
	RootNavigationView().SelectedItem(item);
}

void RootPage::_ProfileService_ProfileRenamed(uint32_t idx) {
	RootNavigationView().MenuItems()
		.GetAt(FIRST_PROFILE_ITEM_IDX + idx)
		.try_as<MUXC::NavigationViewItem>()
		.Content(box_value(AppSettings::Get().Profiles()[idx].name));
}

void RootPage::_ProfileService_ProfileRemoved(uint32_t idx) {
	MUXC::NavigationView nv = RootNavigationView();
	IVector<IInspectable> menuItems = nv.MenuItems();
	nv.SelectedItem(menuItems.GetAt(FIRST_PROFILE_ITEM_IDX - 1));
	menuItems.RemoveAt(FIRST_PROFILE_ITEM_IDX + idx);
}

void RootPage::_ProfileService_ProfileReordered(uint32_t profileIdx, bool isMoveUp) {
	IVector<IInspectable> menuItems = RootNavigationView().MenuItems();

	uint32_t curIdx = FIRST_PROFILE_ITEM_IDX + profileIdx;
	uint32_t otherIdx = isMoveUp ? curIdx - 1 : curIdx + 1;
	
	IInspectable otherItem = menuItems.GetAt(otherIdx);
	menuItems.RemoveAt(otherIdx);
	menuItems.InsertAt(curIdx, otherItem);
}

void RootPage::_UpdateNewProfileNameTextBox(bool fillWithTitle) {
	int idx = _newProfileViewModel->CandidateWindowIndex();
	if (idx < 0) {
		return;
	}

	CandidateWindowItem* selectedItem = get_self<CandidateWindowItem>(
		_newProfileViewModel->CandidateWindows().GetAt(idx).try_as<winrt::Magpie::CandidateWindowItem>());
	hstring text = fillWithTitle ? selectedItem->Title() : selectedItem->DefaultProfileName();

	TextBox textBox = NewProfileNameTextBox();
	if (textBox.Text() == text) {
		return;
	}

	const int size = (int)text.size();
	// 遗憾的是设置 Text 属性会导致撤销/重做历史丢失
	textBox.Text(std::move(text));
	// 修改文本后将光标移到最后
	textBox.Select(size, 0);
	// 如果文本太长，这个调用可以使视口移到光标位置
	textBox.Focus(FocusState::Programmatic);
}

}
