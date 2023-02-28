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

void XamlUtils::RepositionXamlPopups(const winrt::Windows::UI::Xaml::XamlRoot& root, bool closeFlyoutPresenter) {
	for (const auto& popup : winrt::VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		if (closeFlyoutPresenter) {
			auto className = winrt::get_class_name(popup.Child());
			if (className == winrt::name_of<winrt::Controls::FlyoutPresenter>() ||
				className == winrt::name_of<winrt::Controls::MenuFlyoutPresenter>()
				) {
				popup.IsOpen(false);
				continue;
			}
		}

		// 取自 https://github.com/CommunityToolkit/Microsoft.Toolkit.Win32/blob/229fa3cd245ff002906b2a594196b88aded25774/Microsoft.Toolkit.Forms.UI.XamlHost/WindowsXamlHostBase.cs#L180

		// Toggle the CompositeMode property, which will force all windowed Popups
		// to reposition themselves relative to the new position of the host window.
		auto compositeMode = popup.CompositeMode();

		// Set CompositeMode to some value it currently isn't set to.
		if (compositeMode == winrt::ElementCompositeMode::SourceOver) {
			popup.CompositeMode(winrt::ElementCompositeMode::MinBlend);
		} else {
			popup.CompositeMode(winrt::ElementCompositeMode::SourceOver);
		}

		// Restore CompositeMode to whatever it was originally set to.
		popup.CompositeMode(compositeMode);
	}
}

void XamlUtils::UpdateThemeOfTooltips(const DependencyObject& root, ElementTheme theme) {
	if (Win32Utils::GetOSVersion().IsWin11()) {
		// Win11 中 Tooltip 自动适应主题
		return;
	}

	int count = VisualTreeHelper::GetChildrenCount(root);
	for (int i = 0; i < count; ++i) {
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
