#include "pch.h"
#include "CandidateWindowItem.h"
#if __has_include("CandidateWindowItem.g.cpp")
#include "CandidateWindowItem.g.cpp"
#endif
#include "Win32Utils.h"
#include "AppXReader.h"
#include "IconHelper.h"


using namespace winrt;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Graphics::Display;


namespace winrt::Magpie::App::implementation {

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

CandidateWindowItem::CandidateWindowItem(uint64_t hWnd, uint32_t dpi, bool isLightTheme, CoreDispatcher const& dispatcher) {
	_title = Win32Utils::GetWndTitle((HWND)hWnd);
	_defaultProfileName = _title;

	_className = Win32Utils::GetWndClassName((HWND)hWnd);
	_path = Win32Utils::GetPathOfWnd((HWND)hWnd);

	Shapes::Rectangle placeholder;
	placeholder.Width(16);
	placeholder.Height(16);
	_icon = std::move(placeholder);

	_ResolveWindow(true, true, (HWND)hWnd, isLightTheme, dpi, dispatcher);
}

IInspectable CandidateWindowItem::Icon() const noexcept {
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

fire_and_forget CandidateWindowItem::_ResolveWindow(bool resolveIcon, bool resolveName, HWND hWnd, bool isLightTheme, uint32_t dpi, CoreDispatcher dispatcher) {
	assert(resolveIcon || resolveName);

	auto weakThis = get_weak();

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
			[](com_ptr<CandidateWindowItem> that, const std::wstring& defaultProfileName, const std::wstring& aumid, CoreDispatcher const& dispatcher)->fire_and_forget {
				co_await dispatcher.RunAsync(CoreDispatcherPriority::Normal, [that, defaultProfileName(defaultProfileName), aumid(aumid)]() {
					that->_defaultProfileName = defaultProfileName;
					that->_propertyChangedEvent(*that, PropertyChangedEventArgs(L"DefaultProfileName"));
					that->_aumid = aumid;
				});
			}(strongThis, defaultProfileName, reader.AUMID(), dispatcher);
		}
	}

	if (!resolveIcon) {
		co_return;
	}

	const uint32_t iconSize = (uint32_t)std::ceil(dpi * 16 / 96.0);

	SoftwareBitmap iconBitmap{ nullptr };

	if (isPackaged) {
		std::variant<std::wstring, SoftwareBitmap> uwpIcon = reader.GetIcon(iconSize, isLightTheme, true);
		if (uwpIcon.index() == 1) {
			iconBitmap = std::get<1>(uwpIcon);
		}
	} else {
		iconBitmap = IconHelper::GetIconOfWnd(hWnd, iconSize);
	}

	// 切换到主线程
	co_await dispatcher;

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

		strongThis->_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Icon"));
	}
}

}
