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

CandidateWindowItem::CandidateWindowItem(uint64_t hWnd, uint32_t dpi, bool isLightTheme, CoreDispatcher const& dispatcher) 
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

fire_and_forget CandidateWindowItem::_ResolveWindow(bool resolveIcon, bool resolveName) {
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
			[](com_ptr<CandidateWindowItem> that, const std::wstring& defaultProfileName, const std::wstring& aumid, CoreDispatcher const& dispatcher)->fire_and_forget {
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
			co_await strongThis->_SetSoftwareBitmapIconAsync(iconBitmap);
		} else {
			strongThis->_SetDefaultIcon();
		}
	}
}

void CandidateWindowItem::_SetDefaultIcon() {
	FontIcon fontIcon;
	fontIcon.Glyph(L"\uE737");
	fontIcon.FontSize(16);

	_icon = std::move(fontIcon);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Icon"));
}

// 不检查 this 的生命周期，必须在主线程调用
IAsyncAction CandidateWindowItem::_SetSoftwareBitmapIconAsync(Windows::Graphics::Imaging::SoftwareBitmap const& iconBitmap) {
	SoftwareBitmapSource imageSource;
	co_await imageSource.SetBitmapAsync(iconBitmap);

	MUXC::ImageIcon imageIcon;
	imageIcon.Width(16);
	imageIcon.Height(16);
	imageIcon.Source(imageSource);

	_icon = imageIcon;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Icon"));
}

}
