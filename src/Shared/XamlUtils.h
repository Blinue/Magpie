#pragma once
#include <winrt/Windows.UI.Xaml.h>

struct XamlUtils {
	static void CloseComboBoxPopup(const winrt::Windows::UI::Xaml::XamlRoot& root);

	static void ClosePopups(const winrt::Windows::UI::Xaml::XamlRoot& root);

	static void UpdateThemeOfXamlPopups(
		const winrt::Windows::UI::Xaml::XamlRoot& root,
		winrt::Windows::UI::Xaml::ElementTheme theme
	);

	static void RepositionXamlPopups(const winrt::Windows::UI::Xaml::XamlRoot& root, bool closeFlyoutPresenter);

	static void UpdateThemeOfTooltips(
		const winrt::Windows::UI::Xaml::DependencyObject& root,
		winrt::Windows::UI::Xaml::ElementTheme theme
	);

	static bool IsColorLight(const winrt::Windows::UI::Color& clr) noexcept {
		// 来自 https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/apply-windows-themes#know-when-dark-mode-is-enabled
		return 5 * clr.G + 2 * clr.R + clr.B > 8 * 128;
	}
};
