#include "pch.h"
#include "NewProfileDialog.h"
#if __has_include("NewProfileDialog.g.cpp")
#include "NewProfileDialog.g.cpp"
#endif
#if __has_include("CandidateWindow.g.cpp")
#include "CandidateWindow.g.cpp"
#endif
#include "Win32Utils.h"
#include "Utils.h"
#include "ComboBoxHelper.h"
#include "AppSettings.h"
#include "IconHelper.h"
#include "AppXReader.h"
#include "ScalingProfileService.h"
#include <unordered_set>

using namespace winrt;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Graphics::Display;


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
	if (className == L"Progman"	||					// Program Manager
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

static hstring GetProcessDesc(HWND hWnd) {
	if (Win32Utils::GetWndClassName(hWnd) == L"ApplicationFrameWindow") {
		// 跳过 UWP 窗口
		return {};
	}

	// 移植自 https://github.com/dotnet/runtime/blob/4a63cb28b69e1c48bccf592150be7ba297b67950/src/libraries/System.Diagnostics.FileVersionInfo/src/System/Diagnostics/FileVersionInfo.Windows.cs
	std::wstring fileName = Win32Utils::GetPathOfWnd(hWnd);
	if (fileName.empty()) {
		return {};
	}

	DWORD dummy;
	DWORD infoSize = GetFileVersionInfoSizeEx(FILE_VER_GET_LOCALISED, fileName.c_str(), &dummy);
	if (infoSize == 0) {
		return {};
	}

	std::unique_ptr<uint8_t[]> infoData(new uint8_t[infoSize]);
	if (!GetFileVersionInfoEx(FILE_VER_GET_LOCALISED, fileName.c_str(), 0, infoSize, infoData.get())) {
		return {};
	}

	std::wstring codePage;
	uint8_t* langId = nullptr;
	UINT len;
	if (VerQueryValue(infoData.get(), L"\\VarFileInfo\\Translation", (void**)&langId, &len)) {
		codePage = fmt::format(L"{:08X}", uint32_t((*(uint16_t*)langId << 16) | *(uint16_t*)(langId + 2)));
	} else {
		codePage = L"040904E4";
	}

	wchar_t* description = nullptr;
	if (!VerQueryValue(infoData.get(), fmt::format(L"\\StringFileInfo\\{}\\FileDescription", codePage).c_str(), (void**)&description, &len)) {
		return {};
	}

	return description;
}

template<typename It>
static void SortCandidateWindows(It begin, It end) {
	const std::collate<wchar_t>& col = std::use_facet<std::collate<wchar_t> >(std::locale());

	// 根据用户的区域设置排序，对于中文为拼音顺序
	std::sort(begin, end, [&col](Magpie::App::CandidateWindow const& l, Magpie::App::CandidateWindow const& r) {
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

NewProfileDialog::NewProfileDialog() {
	InitializeComponent();

	std::vector<Magpie::App::CandidateWindow> candidateWindows;

	_displayInfomation = DisplayInformation::GetForCurrentView();
	_dpiChangedRevoker = _displayInfomation.DpiChanged(
		auto_revoke, { this, &NewProfileDialog::_DisplayInformation_DpiChanged});

	_colorValuesChangedRevoker = _uiSettings.ColorValuesChanged(
		auto_revoke, { get_weak(), &NewProfileDialog::_UISettings_ColorValuesChanged });

	const UINT dpi = (UINT)std::lroundf(_displayInfomation.LogicalDpi());
	const bool isLightTheme = Application::Current().as<App>().MainPage().ActualTheme() == ElementTheme::Light;
	for (HWND hWnd : GetDesktopWindows()) {
		candidateWindows.emplace_back((uint64_t)hWnd, dpi, isLightTheme, Dispatcher());
	}

	SortCandidateWindows(candidateWindows.begin(), candidateWindows.end());
	_candidateWindows = single_threaded_observable_vector(std::move(candidateWindows));

	std::vector<IInspectable> profiles;
	profiles.push_back(box_value(L"默认"));
	for (const ScalingProfile& profile : AppSettings::Get().ScalingProfiles()) {
		profiles.push_back(box_value(profile.Name()));
	}

	_profiles = single_threaded_vector(std::move(profiles));
}

void NewProfileDialog::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

void NewProfileDialog::RootScrollViewer_SizeChanged(IInspectable const&, IInspectable const&) {
	// 为滚动条预留空间
	if (RootScrollViewer().ScrollableHeight() > 0) {
		RootStackPanel().Padding({ 0,0,18,0 });
	} else {
		RootStackPanel().Padding({});
	}
}

void NewProfileDialog::Loading(FrameworkElement const&, IInspectable const&) {
	_UpdateCandidateWindows();

	_parent = Parent().as<ContentDialog>();
	_parent.IsPrimaryButtonEnabled(false);

	_primaryButtonClickRevoker = _parent.PrimaryButtonClick(auto_revoke, { this, &NewProfileDialog::_ContentDialog_PrimaryButtonClick });
}

void NewProfileDialog::ActualThemeChanged(FrameworkElement const&, IInspectable const&) {
	for (Magpie::App::CandidateWindow const& item : _candidateWindows) {
		item.OnThemeChanged(ActualTheme() == ElementTheme::Light);
	}
}

void NewProfileDialog::CandidateWindowsListView_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&) {
	IInspectable selectedItem = CandidateWindowsListView().SelectedItem();
	if (!selectedItem) {
		ErrorMesageGrid().Visibility(Visibility::Collapsed);
		ProfileNameStackPanel().Visibility(Visibility::Visible);
		ProfileNameTextBox().Text(L"");
		ProfileNameTextBox().IsEnabled(false);
		_parent.IsPrimaryButtonEnabled(false);
		return;
	}

	Magpie::App::CandidateWindow window = selectedItem.as<Magpie::App::CandidateWindow>();

	bool isValidProfile = false;
	{
		HWND hWnd = (HWND)window.HWnd();
		std::wstring className = Win32Utils::GetWndClassName(hWnd);
		hstring aumid = window.AUMID();
		if (!aumid.empty()) {
			isValidProfile = ScalingProfileService::Get().TestNewProfile(true, aumid, className);
		} else {
			isValidProfile = ScalingProfileService::Get().TestNewProfile(false, Win32Utils::GetPathOfWnd(hWnd), className);
		}
	}

	ProfileNameTextBox().Text(window.DefaultProfileName());
	ErrorMesageGrid().Visibility(isValidProfile ? Visibility::Collapsed : Visibility::Visible);
	ProfileNameStackPanel().Visibility(isValidProfile ? Visibility::Visible : Visibility::Collapsed);
	ProfileNameTextBox().IsEnabled(isValidProfile);
	_parent.IsPrimaryButtonEnabled(isValidProfile);
}

void NewProfileDialog::ProfileNameTextBox_TextChanged(IInspectable const&, TextChangedEventArgs const&) {
	_parent.IsPrimaryButtonEnabled(ProfileNameTextBox().IsEnabled() && !ProfileNameTextBox().Text().empty());
}

void NewProfileDialog::_ContentDialog_PrimaryButtonClick(Controls::ContentDialog const&, Controls::ContentDialogButtonClickEventArgs const&) {
	IInspectable selectedItem = CandidateWindowsListView().SelectedItem();
	assert(selectedItem);
	Magpie::App::CandidateWindow window = selectedItem.as<Magpie::App::CandidateWindow>();

	HWND hWnd = (HWND)window.HWnd();
	std::wstring className = Win32Utils::GetWndClassName(hWnd);
	hstring aumid = window.AUMID();
	if (!aumid.empty()) {
		ScalingProfileService::Get().AddProfile(true, aumid, className, ProfileNameTextBox().Text());
	} else {
		ScalingProfileService::Get().AddProfile(false, Win32Utils::GetPathOfWnd(hWnd), className, ProfileNameTextBox().Text());
	}
}

IAsyncAction NewProfileDialog::_UISettings_ColorValuesChanged(UISettings const&, IInspectable const&) {
	co_await Dispatcher();

	for (Magpie::App::CandidateWindow const& item : _candidateWindows) {
		item.OnAccentColorChanged();
	}
}

IAsyncAction NewProfileDialog::_DisplayInformation_DpiChanged(DisplayInformation const&, IInspectable const&) {
	auto weakThis = get_weak();

	// 等待候选窗口更新图标
	co_await std::chrono::milliseconds(100);
	co_await Dispatcher();

	if (auto strongThis = weakThis.get()) {
		const UINT dpi = (UINT)std::lroundf(_displayInfomation.LogicalDpi());
		for (Magpie::App::CandidateWindow const& item : _candidateWindows) {
			item.OnDpiChanged(dpi);
		}
	}
}

static bool IsChanged(const IVector<Magpie::App::CandidateWindow>& oldItems, const std::vector<HWND>& newItems) {
	if (oldItems.Size() != newItems.size()) {
		return true;
	}

	for (Magpie::App::CandidateWindow const& item : oldItems) {
		HWND hwndNew = (HWND)item.HWnd();

		bool flag = false;
		for (HWND hWnd : newItems) {
			if (hWnd == hwndNew) {
				flag = true;
				break;
			}
		}

		if (!flag) {
			return true;
		}
	}

	return false;
}

fire_and_forget NewProfileDialog::_UpdateCandidateWindows() {
	if (_parent) {
		// 不知为何 Loading 会被调用两次
		co_return;
	}

	auto weakThis = get_weak();

	// 更新图标的间隔
	uint32_t count = 0;

	while (true) {
		co_await std::chrono::milliseconds(200);

		auto strongThis = weakThis.get();
		if (!strongThis) {
			co_return;
		}

		co_await Dispatcher();

		++count;

		std::vector<HWND> candiateWindows = GetDesktopWindows();

		if (!IsChanged(_candidateWindows, candiateWindows)) {
			// 更新标题
			for (Magpie::App::CandidateWindow const& item : _candidateWindows) {
				item.UpdateTitle();
			}

			// 更新图标
			if (count >= 5) {
				count = 0;

				for (Magpie::App::CandidateWindow const& item : _candidateWindows) {
					item.UpdateIcon();
				}
			}

			continue;
		}

		const UINT dpi = (UINT)std::lroundf(_displayInfomation.LogicalDpi());
		const bool isLightTheme = ActualTheme() == ElementTheme::Light;

		std::vector<Magpie::App::CandidateWindow> items;
		items.reserve(candiateWindows.size());

		for (HWND hWnd : candiateWindows) {
			bool flag = false;

			for (Magpie::App::CandidateWindow const& item : _candidateWindows) {
				if ((HWND)item.HWnd() == hWnd) {
					items.emplace_back(item, 0);
					flag = true;
					break;
				}
			}

			if (flag) {
				continue;
			}

			items.emplace_back((uint64_t)hWnd, dpi, isLightTheme, Dispatcher());
		}

		SortCandidateWindows(items.begin(), items.end());

		// 计算当前选中的窗口的新位置
		int32_t newSelectIndex = -1;
		IInspectable selectItem = CandidateWindowsListView().SelectedItem();
		if (selectItem) {
			uint64_t selectedHWnd = selectItem.as<Magpie::App::CandidateWindow>().HWnd();
			for (int32_t i = 0, end = (int32_t)items.size(); i < end; ++i) {
				if (items[i].HWnd() == selectedHWnd) {
					newSelectIndex = i;
					break;
				}
			}
		}

		_candidateWindows.ReplaceAll(items);

		if (newSelectIndex >= 0) {
			CandidateWindowsListView().SelectedIndex(newSelectIndex);
		}
	}
}

fire_and_forget CandidateWindow::_ResolveWindow(bool resolveIcon, bool resolveName) {
	assert(resolveIcon || resolveName);

	auto weakThis = get_weak();

	HWND hWnd = (HWND)_hWnd;
	uint32_t dpi = _dpi;
	bool isLightTheme = _isLightTheme;
	CoreDispatcher dispatcher = _dispatcher;
	
	// 解析名称和图标非常耗时，转到后台进行
	co_await resume_background();

	AppXReader reader;
	const bool isPackaged = reader.Initialize(hWnd);
	if (resolveName) {
		std::wstring defaultProfileName = isPackaged ? reader.GetDisplayName() : (std::wstring)GetProcessDesc(hWnd);

		auto strongThis = weakThis.get();
		if (!strongThis) {
			co_return;
		}

		if (!defaultProfileName.empty()) {
			[](com_ptr<CandidateWindow> that, const std::wstring& defaultProfileName, const std::wstring& aumid, CoreDispatcher const& dispatcher)->fire_and_forget {
				co_await dispatcher.RunAsync(CoreDispatcherPriority::Normal, [that, defaultProfileName(defaultProfileName), aumid(aumid)]() {
					that->_defaultProfileName = defaultProfileName;
					that->_aumid = aumid;
				});
			}(strongThis, defaultProfileName, reader.AUMID(), dispatcher);
		}
	}
	
	if (!resolveIcon) {
		co_return;
	}

	const uint32_t iconSize = (uint32_t)std::ceil(dpi * 16 / 96.0);

	std::wstring iconPath;
	SoftwareBitmap iconBitmap{ nullptr };

	if (isPackaged) {
		std::variant<std::wstring, SoftwareBitmap> uwpIcon = reader.GetIcon(iconSize, isLightTheme);
		if (uwpIcon.index() == 0) {
			iconPath = std::get<0>(uwpIcon);
		} else {
			iconBitmap = std::get<1>(uwpIcon);
		}
	} else {
		iconBitmap = IconHelper::GetIconOfWnd(hWnd, iconSize);
	}

	// 切换到主线程
	co_await dispatcher;

	if (auto strongThis = weakThis.get()) {
		if (!iconPath.empty()) {
			strongThis->_SetIconPath(iconPath);
		} else if (iconBitmap) {
			co_await strongThis->_SetSoftwareBitmapIconAsync(iconBitmap);
		} else {
			strongThis->_SetDefaultIcon();
		}
	}
}

CandidateWindow::CandidateWindow(uint64_t hWnd, uint32_t dpi, bool isLightTheme, CoreDispatcher const& dispatcher)
	: _hWnd(hWnd), _dispatcher(dispatcher), _dpi(dpi), _isLightTheme(isLightTheme)
{
	_title = Win32Utils::GetWndTitle((HWND)hWnd);
	_defaultProfileName = _title;

	Shapes::Rectangle placeholder;
	placeholder.Width(16);
	placeholder.Height(16);
	_icon = std::move(placeholder);

	_ResolveWindow(true, true);
}

CandidateWindow::CandidateWindow(Magpie::App::CandidateWindow const& other, int) {
	CandidateWindow* otherImpl = get_self<CandidateWindow>(other);
	_hWnd = otherImpl->_hWnd;
	_dispatcher = otherImpl->_dispatcher;
	_dpi = otherImpl->_dpi;
	_isLightTheme = otherImpl->_isLightTheme;
	_title = otherImpl->_title;
	_defaultProfileName = otherImpl->_defaultProfileName;
	_aumid = otherImpl->_aumid;

	// 复制图标不能直接复制引用
	if (MUXC::ImageIcon imageIcon = otherImpl->_icon.try_as<MUXC::ImageIcon>()) {
		MUXC::ImageIcon icon;
		icon.Source(imageIcon.Source());
		icon.Width(imageIcon.Width());
		icon.Height(imageIcon.Height());
		_icon = std::move(icon);
	} else if (BitmapIcon bitmapIcon = otherImpl->_icon.try_as<BitmapIcon>()) {
		BitmapIcon icon;
		icon.ShowAsMonochrome(bitmapIcon.ShowAsMonochrome());
		icon.UriSource(bitmapIcon.UriSource());
		icon.Width(bitmapIcon.Width());
		icon.Height(bitmapIcon.Height());
		_icon = std::move(icon);
	} else if (FontIcon fontIcon = otherImpl->_icon.try_as<FontIcon>()) {
		FontIcon icon;
		icon.Glyph(fontIcon.Glyph());
		icon.FontSize(fontIcon.FontSize());
		_icon = std::move(icon);
	}
}

void CandidateWindow::UpdateTitle() {
	_title = Win32Utils::GetWndTitle((HWND)_hWnd);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Title"));
}

void CandidateWindow::UpdateIcon() {
	if (!_aumid.empty()) {
		// 打包应用无需更新图标
		return;
	}

	[](CandidateWindow* that)->fire_and_forget {
		HWND hWnd = (HWND)that->_hWnd;
		uint32_t iconSize = (uint32_t)std::ceil(that->_dpi * 16 / 96.0);
		CoreDispatcher dispatcher = that->_dispatcher;

		auto weakThis = that->get_weak();

		co_await resume_background();

		SoftwareBitmap iconBitmap = IconHelper::GetIconOfWnd(hWnd, iconSize);

		co_await dispatcher;

		if (auto strongThis = weakThis.get()) {
			co_await strongThis->_SetSoftwareBitmapIconAsync(iconBitmap);
		}
	}(this);
}

void CandidateWindow::OnThemeChanged(bool isLightTheme) {
	_isLightTheme = isLightTheme;
	_ResolveWindow(true, false);
}

void CandidateWindow::OnAccentColorChanged() {
	if (_aumid.empty() || get_class_name(_icon) != name_of<MUXC::ImageIcon>()) {
		return;
	}

	_ResolveWindow(true, false);
}

void CandidateWindow::OnDpiChanged(uint32_t newDpi) {
	_dpi = newDpi;
	_ResolveWindow(true, false);
}

void CandidateWindow::_SetDefaultIcon() {
	FontIcon fontIcon;
	fontIcon.Glyph(L"\uE737");
	fontIcon.FontSize(16);

	_icon = std::move(fontIcon);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Icon"));
}

// 不检查 this 的生命周期，必须在主线程调用
IAsyncAction CandidateWindow::_SetSoftwareBitmapIconAsync(SoftwareBitmap const& iconBitmap) {
	SoftwareBitmapSource imageSource;
	co_await imageSource.SetBitmapAsync(iconBitmap);

	MUXC::ImageIcon imageIcon;
	imageIcon.Width(16);
	imageIcon.Height(16);
	imageIcon.Source(imageSource);

	_icon = imageIcon;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Icon"));
}

void CandidateWindow::_SetIconPath(std::wstring_view iconPath) {
	BitmapIcon icon;
	icon.ShowAsMonochrome(false);
	icon.UriSource(Uri(iconPath));
	icon.Width(16);
	icon.Height(16);

	_icon = std::move(icon);

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Icon"));
}

}
