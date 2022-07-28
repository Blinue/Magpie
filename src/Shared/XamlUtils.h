#pragma once
#include "CommonPCH.h"
#include <winrt/Windows.UI.Xaml.h>


struct XamlUtils {
	static void CloseXamlPopups(const winrt::Windows::UI::Xaml::XamlRoot& root);

	static void UpdateThemeOfXamlPopups(
		const winrt::Windows::UI::Xaml::XamlRoot& root,
		winrt::Windows::UI::Xaml::ElementTheme theme
	);

	static void UpdateThemeOfTooltips(
		const winrt::Windows::UI::Xaml::DependencyObject& root,
		winrt::Windows::UI::Xaml::ElementTheme theme
	);

	static winrt::Windows::UI::Color Win32ColorToWinRTColor(COLORREF color) {
		return { 255, GetRValue(color), GetGValue(color), GetBValue(color) };
	}
};