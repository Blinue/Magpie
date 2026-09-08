#pragma once

namespace Magpie {

struct XamlHelper {
	// 使 XAML Islands 背景透明
	static void SetWindowBackgroundTransparency(const winrt::Window& window, bool transparent) noexcept;

	static void CloseComboBoxPopup(const winrt::XamlRoot& root);

	static void ClosePopups(const winrt::XamlRoot& root);

	static void UpdateThemeOfXamlPopups(const winrt::XamlRoot& root, winrt::ElementTheme theme);

	static void RepositionXamlPopups(const winrt::XamlRoot& root, bool closeFlyoutPresenter);

	static void UpdateThemeOfTooltips(const winrt::DependencyObject& root, winrt::ElementTheme theme);

	static bool ContainsControl(const winrt::DependencyObject& parent, const winrt::DependencyObject& target);

	static bool IsNullOrEmptyString(const winrt::IInspectable& value) noexcept;
};

}
