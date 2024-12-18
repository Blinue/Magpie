#pragma once

namespace Magpie {

struct XamlHelper {
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
};

}
