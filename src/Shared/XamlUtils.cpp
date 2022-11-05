#include "pch.h"
#include "XamlUtils.h"
#include "Win32Utils.h"
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>


using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;


void XamlUtils::CloseXamlPopups(const XamlRoot& root) {
	if (!root) {
		return;
	}

	for (const auto& popup : VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		winrt::hstring className = winrt::get_class_name(popup.Child());
		if (className == winrt::name_of<ContentDialog>() || className == winrt::name_of<Shapes::Rectangle>()) {
			continue;
		}

		popup.IsOpen(false);
	}
}

void XamlUtils::CloseContentDialog(const winrt::Windows::UI::Xaml::XamlRoot& root) {
	if (!root) {
		return;
	}

	for (const auto& popup : VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		if (ContentDialog dialog = popup.Child().try_as<ContentDialog>()) {
			dialog.Hide();
			return;
		}
	}
}

bool XamlUtils::IsAnyContentDialogOpen(const winrt::Windows::UI::Xaml::XamlRoot& root) {
	if (!root) {
		return false;
	}

	for (const auto& popup : VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		if (winrt::get_class_name(popup.Child()) == winrt::name_of<ContentDialog>()) {
			return true;
		}
	}

	return false;
}

void XamlUtils::UpdateThemeOfXamlPopups(const XamlRoot& root, ElementTheme theme) {
	if (!root) {
		return;
	}

	for (const auto& popup : VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		FrameworkElement child = popup.Child().as<FrameworkElement>();
		child.RequestedTheme(theme);
		UpdateThemeOfTooltips(child, theme);
	}
}

void XamlUtils::UpdateThemeOfTooltips(const DependencyObject& root, ElementTheme theme) {
	if (Win32Utils::GetOSBuild() >= 22000) {
		// Win11 中 Tooltip 自动适应主题
		return;
	}

	int32_t count = VisualTreeHelper::GetChildrenCount(root);
	for (int32_t i = 0; i < count; ++i) {
		DependencyObject current = VisualTreeHelper::GetChild(root, i);

		IInspectable tooltipContent = ToolTipService::GetToolTip(current);
		if (tooltipContent) {
			ToolTip tooltip = tooltipContent.try_as<ToolTip>();
			if (tooltip) {
				tooltip.RequestedTheme(theme);
			} else {
				hstring str = winrt::get_class_name(current);
				ToolTip themedTooltip;
				themedTooltip.Content(tooltipContent);
				themedTooltip.RequestedTheme(theme);
				ToolTipService::SetToolTip(current, themedTooltip);
			}
		}

		UpdateThemeOfTooltips(current, theme);
	}
}
