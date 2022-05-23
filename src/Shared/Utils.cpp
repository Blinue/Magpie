#include "Utils.h"
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;


UINT Utils::GetOSBuild() {
	static UINT build = 0;

	if (build == 0) {
		HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
		if (!hNtDll) {
			return {};
		}

		auto rtlGetVersion = (LONG(WINAPI*)(PRTL_OSVERSIONINFOW))GetProcAddress(hNtDll, "RtlGetVersion");
		if (rtlGetVersion == nullptr) {
			return {};
		}

		OSVERSIONINFOW version{};
		version.dwOSVersionInfoSize = sizeof(version);
		rtlGetVersion(&version);

		build = version.dwBuildNumber;
	}

	return build;
}

void Utils::CloseAllXamlPopups(XamlRoot root) {
	if (!root) {
		return;
	}

	// https://github.com/microsoft/microsoft-ui-xaml/issues/4554
	for (const auto& popup : VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		popup.IsOpen(false);
	}
}

void Utils::UpdateThemeOfXamlPopups(XamlRoot root, ElementTheme theme) {
	if (!root) {
		return;
	}

	ElementTheme oppositeTheme = theme == ElementTheme::Light ? ElementTheme::Dark : ElementTheme::Light;

	for (const auto& popup : VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		// 这样可以确保主题被应用
		popup.RequestedTheme(oppositeTheme);
		popup.RequestedTheme(theme);
	}
}
