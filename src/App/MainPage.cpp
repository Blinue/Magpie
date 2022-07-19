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


using namespace winrt;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media::Imaging;


namespace winrt::Magpie::App::implementation {

static bool IsCandidateWindow(HWND hWnd) {
	if (!IsWindowVisible(hWnd)) {
		return false;
	}

	RECT frameRect;
	if (!Win32Utils::GetWindowFrameRect(hWnd, frameRect)) {
		return false;
	}

	SIZE frameSize = Win32Utils::GetSizeOfRect(frameRect);
	if (frameSize.cx < 50 && frameSize.cy < 50) {
		return false;
	}

	// 标题不能为空
	if (Win32Utils::GetWndTitle(hWnd).empty()) {
		return false;
	}

	// 排除后台 UWP 窗口
	// https://stackoverflow.com/questions/43927156/enumwindows-returns-closed-windows-store-applications
	UINT isCloaked{};
	DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &isCloaked, sizeof(isCloaked));
	if (isCloaked != 0) {
		return false;
	}

	std::wstring className = Win32Utils::GetWndClassName(hWnd);
	if (className == L"Progman" ||					// Program Manager
		className == L"Xaml_WindowedPopupClass"		// 主机弹出窗口
	) {
		return false;
	}

	return true;
}

static std::vector<HWND> GetDesktopWindows() {
	std::vector<HWND> windows;
	windows.reserve(1024);

	// EnumWindows 能否枚举到 UWP 窗口？
	// 虽然官方文档中明确指出不能，但我在 Win10/11 中测试是可以的
	// PowerToys 也依赖这个行为 https://github.com/microsoft/PowerToys/blob/d4b62d8118d49b2cc83c2a2126091378d0b5fec4/src/modules/launcher/Plugins/Microsoft.Plugin.WindowWalker/Components/OpenWindows.cs
	// 无法枚举到全屏状态下的 UWP 窗口
	EnumWindows(
		[](HWND hWnd, LPARAM lParam) {
			std::vector<HWND>& windows = *(std::vector<HWND>*)lParam;

			if (IsCandidateWindow(hWnd)) {
				windows.push_back(hWnd);
			}

			return TRUE;
		},
		(LPARAM)&windows
			);

	return windows;
}

template<typename It>
static void SortCandidateWindows(It begin, It end) {
	const std::collate<wchar_t>& col = std::use_facet<std::collate<wchar_t> >(std::locale());

	// 根据用户的区域设置排序，对于中文为拼音顺序
	std::sort(begin, end, [&col](Magpie::App::CandidateWindowItem const& l, Magpie::App::CandidateWindowItem const& r) {
		const hstring& titleL = l.Title();
		const hstring& titleR = r.Title();

		// 忽略大小写，忽略半/全角
		return CompareStringEx(
			LOCALE_NAME_USER_DEFAULT,
			NORM_IGNORECASE | NORM_IGNOREWIDTH | NORM_LINGUISTIC_CASING,
			titleL.c_str(), titleL.size(),
			titleR.c_str(), titleR.size(),
			nullptr, nullptr, 0
		) == CSTR_LESS_THAN;
	});
}

MainPage::MainPage() {
	InitializeComponent();

	_UpdateTheme(false);
	AppSettings::Get().ThemeChanged([this](int) { _UpdateTheme(); });

	_colorChangedRevoker = _uiSettings.ColorValuesChanged(
		auto_revoke, { get_weak(), &MainPage::_UISettings_ColorValuesChanged });

	Background(MicaBrush(*this));

	_displayInfomation = DisplayInformation::GetForCurrentView();

	IVector<IInspectable> navMenuItems = __super::RootNavigationView().MenuItems();
	for (const ScalingProfile& profile : AppSettings::Get().ScalingProfiles()) {
		MUXC::NavigationViewItem item;
		item.Content(box_value(profile.Name()));
		_LoadIcon(item, profile);

		navMenuItems.InsertAt(navMenuItems.Size() - 1, item);
	}

	// Win10 里启动时有一个 ToggleSwitch 的动画 bug，这里展示页面切换动画掩盖
	if (Win32Utils::GetOSBuild() < 22000) {
		ContentFrame().Navigate(winrt::xaml_typename<Controls::Page>());
	}

	_profileAddedRevoker = ScalingProfileService::Get().ProfileAdded(
		auto_revoke, { this, &MainPage::_ScalingProfileService_ProfileAdded });
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

void MainPage::AddNavigationViewItem_Tapped(IInspectable const&, TappedRoutedEventArgs const&) {
	const UINT dpi = (UINT)std::lroundf(_displayInfomation.LogicalDpi());
	const bool isLightTheme = ActualTheme() == ElementTheme::Light;

	std::vector<CandidateWindowItem> candidateWindows;
	for (HWND hWnd : GetDesktopWindows()) {
		candidateWindows.emplace_back((uint64_t)hWnd, dpi, isLightTheme, Dispatcher());
	}

	SortCandidateWindows(candidateWindows.begin(), candidateWindows.end());

	std::vector<IInspectable> items;
	items.reserve(candidateWindows.size());
	std::copy(candidateWindows.begin(), candidateWindows.end(), std::insert_iterator(items, items.begin()));
	_candidateWindows = single_threaded_vector(std::move(items));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CandidateWindows"));

	std::vector<IInspectable> profiles;
	profiles.push_back(box_value(L"默认"));
	for (const ScalingProfile& profile : AppSettings::Get().ScalingProfiles()) {
		profiles.push_back(box_value(profile.Name()));
	}

	_profiles = single_threaded_vector(std::move(profiles));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Profiles"));

	ProfileIndex(0);

	FlyoutBase::ShowAttachedFlyout(AddNavigationViewItem());
}

MUXC::NavigationView MainPage::RootNavigationView() {
	return __super::RootNavigationView();
}

void MainPage::ComboBox_DropDownOpened(IInspectable const&, IInspectable const&) {
	XamlUtils::UpdateThemeOfXamlPopups(XamlRoot(), ActualTheme());
}

void MainPage::_UpdateTheme(bool updateIcons) {
	int theme = AppSettings::Get().Theme();

	bool isDarkTheme = FALSE;
	if (theme == 2) {
		isDarkTheme = _uiSettings.GetColorValue(UIColorType::Background).R < 128;
	} else {
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

	if (updateIcons && IsLoaded()) {
		_UpdateUWPIcons();
	}

	Logger::Get().Info(StrUtils::Concat("当前主题：", isDarkTheme ? "深色" : "浅色"));
}

fire_and_forget MainPage::_LoadIcon(MUXC::NavigationViewItem const& item, const ScalingProfile& profile) {
	// 用于占位
	item.Icon(FontIcon());

	weak_ref<MUXC::NavigationViewItem> weakRef(item);

	bool preferLightTheme = item.ActualTheme() == ElementTheme::Light;
	bool isPackaged = profile.IsPackaged();
	std::wstring path = profile.PathRule();
	CoreDispatcher dispatcher = Dispatcher();
	uint32_t dpi = (uint32_t)std::lroundf(_displayInfomation.LogicalDpi());
	uint32_t preferredIconSize = (uint32_t)std::ceil(dpi * 16 / 96.0);

	co_await resume_background();

	std::wstring iconPath;
	SoftwareBitmap iconBitmap{ nullptr };

	if (isPackaged) {
		AppXReader reader;
		reader.Initialize(path);

		std::variant<std::wstring, SoftwareBitmap> uwpIcon = reader.GetIcon(preferredIconSize, preferLightTheme);
		if (uwpIcon.index() == 0) {
			iconPath = std::get<0>(uwpIcon);
		} else {
			iconBitmap = std::get<1>(uwpIcon);
		}
	} else {
		iconBitmap = IconHelper::GetIconOfExe(path.c_str(), preferredIconSize);
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

IAsyncAction MainPage::_UISettings_ColorValuesChanged(Windows::UI::ViewManagement::UISettings const&, IInspectable const&) {
	co_await Dispatcher();

	if (AppSettings::Get().Theme() == 2) {
		_UpdateTheme(false);
	}
	
	_UpdateUWPIcons();
}

void MainPage::_UpdateUWPIcons() {
	IVector<IInspectable> navMenuItems = __super::RootNavigationView().MenuItems();
	const std::vector<ScalingProfile>& profiles = AppSettings::Get().ScalingProfiles();
	const uint32_t firstProfileIdx = navMenuItems.Size() - (uint32_t)profiles.size() - 1;

	for (uint32_t i = 0; i < profiles.size(); ++i) {
		if (!profiles[i].IsPackaged()) {
			continue;
		}

		MUXC::NavigationViewItem item = navMenuItems.GetAt(firstProfileIdx + i).as<MUXC::NavigationViewItem>();
		_LoadIcon(item, profiles[i]);
	}
}

void MainPage::_ScalingProfileService_ProfileAdded(ScalingProfile& profile) {
	MUXC::NavigationViewItem item;
	item.Content(box_value(profile.Name()));
	_LoadIcon(item, profile);

	IVector<IInspectable> navMenuItems = __super::RootNavigationView().MenuItems();
	navMenuItems.InsertAt(navMenuItems.Size() - 1, item);
	__super::RootNavigationView().SelectedItem(item);
}

} // namespace winrt::Magpie::implementation
