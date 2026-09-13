#include "pch.h"
#include "CandidateWindowItem.h"
#if __has_include("CandidateWindowItem.g.cpp")
#include "CandidateWindowItem.g.cpp"
#endif
#include "App.h"
#include "AppXReader.h"
#include "ByteBuffer.h"
#include "IconHelper.h"
#include "MainWindow.h"
#include "StrHelper.h"
#include "Win32Helper.h"

using namespace ::Magpie;
using namespace winrt;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Graphics::Display;

namespace winrt::Magpie::implementation {

CandidateWindowItem::CandidateWindowItem(HWND hWnd) {
	_title = Win32Helper::GetWindowTitle(hWnd);
	_defaultProfileName = _title;

	_className = Win32Helper::GetWindowClassName(hWnd);
	_path = Win32Helper::GetWindowExePath(hWnd);

	MUXC::ImageIcon placeholder;
	placeholder.Width(16);
	placeholder.Height(16);
	_icon = std::move(placeholder);

	_ResolveWindow(true, true, hWnd);
}

IconElement CandidateWindowItem::Icon() const noexcept {
	// 返回副本，否则在 ComboBox 中绑定会导致崩溃
	if (MUXC::ImageIcon imageIcon = _icon.try_as<MUXC::ImageIcon>()) {
		MUXC::ImageIcon icon;
		icon.Source(imageIcon.Source());
		icon.Width(imageIcon.Width());
		icon.Height(imageIcon.Height());
		return std::move(icon);
	} else if (FontIcon fontIcon = _icon.try_as<FontIcon>()) {
		FontIcon icon;
		icon.Glyph(fontIcon.Glyph());
		icon.FontSize(fontIcon.FontSize());
		return std::move(icon);
	}

	return nullptr;
}

fire_and_forget CandidateWindowItem::_ResolveWindow(bool resolveIcon, bool resolveName, HWND hWnd) {
	assert(resolveIcon || resolveName);

	auto weakThis = get_weak();

	// 解析名称和图标非常耗时，转到后台进行
	co_await resume_background();

	AppXReader reader;
	const bool isPackaged = reader.Initialize(hWnd);
	if (resolveName) {
		std::wstring defaultProfileName =
			isPackaged ? reader.GetDisplayName() : Win32Helper::GetProcessDescriptionFromWindow(hWnd);
		StrHelper::Trim(defaultProfileName);

		auto strongThis = weakThis.get();
		if (!strongThis) {
			co_return;
		}

		App::Get().Dispatcher().TryEnqueue(
			[this, defaultProfileName(std::move(defaultProfileName)), aumid(reader.AUMID())]() {
				if (!defaultProfileName.empty()) {
					_defaultProfileName = defaultProfileName;
				}
				// 即使 defaultProfileName 为空也通知 DefaultProfileName 已更改，
				// 这是为了正确设置 CandidateWindowIndex。
				RaisePropertyChanged(L"DefaultProfileName");

				_aumid = aumid;
			}
		);
	}

	if (!resolveIcon) {
		co_return;
	}

	SoftwareBitmap iconBitmap{ nullptr };
	const uint32_t iconSize = (uint32_t)std::lround(
		16 * App::Get().MainWindow().GetDpi() / double(USER_DEFAULT_SCREEN_DPI));

	if (isPackaged) {
		std::variant<std::wstring, SoftwareBitmap> uwpIcon =
			reader.GetIcon(iconSize, App::Get().IsLightTheme(), true);
		if (uwpIcon.index() == 1) {
			iconBitmap = std::get<1>(uwpIcon);
		}
	} else {
		iconBitmap = IconHelper::ExtractIconFromWindow(hWnd, iconSize);
	}

	// 切换到主线程
	co_await App::Get().Dispatcher();

	if (auto strongThis = weakThis.get()) {
		if (iconBitmap) {
			SoftwareBitmapSource imageSource;
			co_await imageSource.SetBitmapAsync(iconBitmap);

			MUXC::ImageIcon imageIcon;
			imageIcon.Width(16);
			imageIcon.Height(16);
			imageIcon.Source(imageSource);

			strongThis->_icon = std::move(imageIcon);
		} else {
			FontIcon fontIcon;
			fontIcon.Glyph(L"\uE737");
			fontIcon.FontSize(16);

			strongThis->_icon = std::move(fontIcon);
		}

		strongThis->RaisePropertyChanged(L"Icon");
	}
}

}
