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

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;


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

static HICON GetIconOfWnd(HWND hWnd) {
	HICON result = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
	if (result) {
		return result;
	}

	result = (HICON)SendMessage(hWnd, WM_GETICON, ICON_BIG, 0);
	if (result) {
		return result;
	}

	result = (HICON)GetClassLongPtr(hWnd, GCLP_HICONSM);
	if (result) {
		return result;
	}

	result = (HICON)GetClassLongPtr(hWnd, GCLP_HICON);
	if (result) {
		return result;
	}

	// 此窗口无图标则回落到所有者窗口
	HWND hwndOwner = GetWindow(hWnd, GW_OWNER);
	if (!hwndOwner) {
		return NULL;
	}

	return GetIconOfWnd(hwndOwner);
}

static IAsyncOperation<IInspectable> ResolveWindowIconAsync(HWND hWnd) {
	HICON hIcon = GetIconOfWnd(hWnd);
	if (hIcon) {
		ImageSource imageSource = co_await IconHelper::HIcon2ImageSourceAsync(hIcon);

		if (imageSource) {
			MUXC::ImageIcon icon;
			icon.Width(16);
			icon.Height(16);
			icon.Source(imageSource);
			co_return icon;
		}
	}

	// 回落到通用图标
	FontIcon icon;
	icon.Glyph(L"\uE737");
	icon.FontSize(16);
	co_return icon;
}

static hstring GetProcessDesc(HWND hWnd) {
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

	for (HWND hWnd : GetDesktopWindows()) {
		std::wstring title = Win32Utils::GetWndTitle(hWnd);
		if (title.empty()) {
			continue;
		}

		Magpie::App::CandidateWindow cw;
		cw.HWnd((uint64_t)hWnd);
		cw.Title(title);

		candidateWindows.emplace_back(cw);
	}

	_candidateWindows = single_threaded_observable_vector(std::move(candidateWindows));

	[this]() -> fire_and_forget {
		for (const auto& item : _candidateWindows) {
			item.Icon(co_await ResolveWindowIconAsync((HWND)item.HWnd()));
		}
	}();

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

	const Magpie::App::CandidateWindow& item = _candidateWindows.GetAt(_windowIndex);
	hstring processDesc = GetProcessDesc((HWND)item.HWnd());
	// 如果不存在文件描述回落到标题
	NameTextBox().Text(processDesc.empty() ? item.Title() : processDesc);
}

void NewProfileDialog::RootScrollViewer_SizeChanged(IInspectable const&, IInspectable const&) {
	// 为滚动条预留空间
	if (RootScrollViewer().ScrollableHeight() > 0) {
		RootStackPanel().Padding({ 0,0,18,0 });
	} else {
		RootStackPanel().Padding({});
	}
}

}
