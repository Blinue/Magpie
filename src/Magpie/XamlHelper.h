#pragma once

namespace Magpie {

struct XamlHelper {
	static void CloseComboBoxPopup(const winrt::XamlRoot& root);

	static void ClosePopups(const winrt::XamlRoot& root);

	static void UpdateThemeOfXamlPopups(const winrt::XamlRoot& root, winrt::ElementTheme theme);

	static void RepositionXamlPopups(const winrt::XamlRoot& root, bool closeFlyoutPresenter);

	static void UpdateThemeOfTooltips(const winrt::DependencyObject& root, winrt::ElementTheme theme);
};

}
