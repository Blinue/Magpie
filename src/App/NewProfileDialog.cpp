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
#include "AppXHelper.h"

using namespace winrt;
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

	// 排除后台 UWP 窗口
	// https://stackoverflow.com/questions/43927156/enumwindows-returns-closed-windows-store-applications
	UINT isCloaked{};
	DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &isCloaked, sizeof(isCloaked));
	if (isCloaked != 0) {
		return false;
	}

	if (Win32Utils::GetWndClassName(hWnd) == L"Progman") {
		// 排除 Program Manager 窗口
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

NewProfileDialog::NewProfileDialog() {
	InitializeComponent();

	std::vector<Magpie::App::CandidateWindow> candidateWindows;

	const UINT dpi = (UINT)std::lroundf(DisplayInformation::GetForCurrentView().LogicalDpi());
	for (HWND hWnd : GetDesktopWindows()) {
		std::wstring title = Win32Utils::GetWndTitle(hWnd);
		if (title.empty()) {
			continue;
		}

		candidateWindows.emplace_back(Magpie::App::CandidateWindow((uint64_t)hWnd, dpi));
	}

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

void NewProfileDialog::WindowIndex(int32_t value) noexcept {
	_windowIndex = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"WindowIndex"));
}

void NewProfileDialog::RootScrollViewer_SizeChanged(IInspectable const&, IInspectable const&) {
	// 为滚动条预留空间
	if (RootScrollViewer().ScrollableHeight() > 0) {
		RootStackPanel().Padding({ 0,0,18,0 });
	} else {
		RootStackPanel().Padding({});
	}
}

CandidateWindow::CandidateWindow(uint64_t hWnd, uint32_t dpi) {
	_hWnd = hWnd;

	_title = Win32Utils::GetWndTitle((HWND)hWnd);
	_defaultProfileName = _title;

	Shapes::Rectangle placeholder;
	placeholder.Width(16);
	placeholder.Height(16);
	_icon = std::move(placeholder);

	([](weak_ref<CandidateWindow> weakThis, uint32_t dpi) -> fire_and_forget {
		HWND hWnd = (HWND)weakThis.get()->HWnd();

		apartment_context uiThread;
		co_await resume_background();

		std::wstring defaultProfileName;
		ImageSource bitmapSource{ nullptr };
		SoftwareBitmap iconBitmap{ nullptr };

		AppXHelper::AppXReader reader;
		const bool isPackaged = reader.Initialize(hWnd);
		if (isPackaged) {
			// 打包应用
			defaultProfileName = reader.GetDisplayName();
			bitmapSource = reader.GetIcon({16,16});
		} else {
			// Win32 窗口
			defaultProfileName = GetProcessDesc(hWnd);

			LONG iconSize = (LONG)std::ceil(dpi * 16 / 96.0);
			iconBitmap = co_await IconHelper::GetIconOfWndAsync(hWnd, { iconSize, iconSize });
		}

		co_await uiThread;

		auto strongThis = weakThis.get();
		if (!strongThis) {
			co_return;
		}

		if (!defaultProfileName.empty()) {
			strongThis->_defaultProfileName = defaultProfileName;
		}

		IInspectable icon{ nullptr };
		if (bitmapSource) {
			MUXC::ImageIcon imageIcon;
			imageIcon.Width(16);
			imageIcon.Height(16);
			imageIcon.Source(bitmapSource);

			icon = std::move(imageIcon);
		} else if (iconBitmap) {
			// 必须在主线程构造
			SoftwareBitmapSource imageSource;
			co_await imageSource.SetBitmapAsync(iconBitmap);

			MUXC::ImageIcon imageIcon;
			imageIcon.Width(16);
			imageIcon.Height(16);
			imageIcon.Source(imageSource);

			icon = std::move(imageIcon);
		} else {
			// 回落到通用图标
			FontIcon fontIcon;
			fontIcon.Glyph(L"\uE737");
			fontIcon.FontSize(16);

			icon = std::move(fontIcon);
		}

		strongThis->_Icon(icon);
	})(get_weak(), dpi);
}

}
