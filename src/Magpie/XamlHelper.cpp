#include "pch.h"
#include "XamlHelper.h"
#include "Win32Helper.h"
#include "SmallVector.h"
#include <inspectable.h>

namespace winrt {
using namespace Windows::UI::Xaml::Controls::Primitives;
}

// 来自 https://learn.microsoft.com/en-us/windows/apps/api-reference/interface-members/ixamlsourcetransparency-isbackgroundtransparent
DECLARE_INTERFACE_IID_(IXamlSourceTransparency, IInspectable, "06636C29-5A17-458D-8EA2-2422D997A922") {
	STDMETHOD(get_IsBackgroundTransparent)(boolean* value) PURE;
	STDMETHOD(put_IsBackgroundTransparent)(boolean value) PURE;
};

namespace Magpie {

void XamlHelper::SetWindowBackgroundTransparency(const winrt::Window& window, bool transparent) noexcept {
	if (auto xst = window.try_as<IXamlSourceTransparency>()) {
		xst->put_IsBackgroundTransparent(transparent);
	}
}

static bool IsComboBoxPopup(const winrt::Popup& popup) {
	winrt::UIElement child = popup.Child();
	if (!child.try_as<winrt::Canvas>()) {
		return false;
	}

	// 查找 XAML 树中是否存在 ComboBoxItem
	SmallVector<winrt::DependencyObject> elems{ std::move(child) };
	do {
		SmallVector<winrt::DependencyObject> temp;

		for (const winrt::DependencyObject& elem : elems) {
			const int count = winrt::VisualTreeHelper::GetChildrenCount(elem);
			for (int i = 0; i < count; ++i) {
				winrt::DependencyObject current = winrt::VisualTreeHelper::GetChild(elem, i);

				if (current.try_as<winrt::ComboBoxItem>()) {
					return true;
				}

				temp.emplace_back(std::move(current));
			}
		}

		elems = std::move(temp);
	} while (!elems.empty());

	return false;
}

void XamlHelper::CloseComboBoxPopup(const winrt::XamlRoot& root) {
	for (const winrt::Popup& popup : winrt::VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		if (IsComboBoxPopup(popup)) {
			popup.IsOpen(false);
			return;
		}
	}
}

void XamlHelper::ClosePopups(const winrt::XamlRoot& root) {
	for (const auto& popup : winrt::VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		popup.IsOpen(false);
	}
}

void XamlHelper::UpdateThemeOfXamlPopups(const winrt::XamlRoot& root, winrt::ElementTheme theme) {
	if (!root) {
		return;
	}

	for (const auto& popup : winrt::VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		winrt::FrameworkElement child = popup.Child().try_as<winrt::FrameworkElement>();
		child.RequestedTheme(theme);
		UpdateThemeOfTooltips(child, theme);
	}
}

void XamlHelper::RepositionXamlPopups(const winrt::Windows::UI::Xaml::XamlRoot& root, bool closeFlyoutPresenter) {
	for (const auto& popup : winrt::VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		if (closeFlyoutPresenter) {
			auto className = winrt::get_class_name(popup.Child());
			if (className == winrt::name_of<winrt::FlyoutPresenter>() ||
				className == winrt::name_of<winrt::MenuFlyoutPresenter>()) {
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

void XamlHelper::UpdateThemeOfTooltips(const winrt::DependencyObject& root, winrt::ElementTheme theme) {
	if (Win32Helper::GetOSVersion().IsWin11()) {
		// Win11 中 Tooltip 自动适应主题
		return;
	}

	// 遍历 XAML 树
	SmallVector<winrt::DependencyObject> elems{ root };
	do {
		SmallVector<winrt::DependencyObject> temp;

		for (const winrt::DependencyObject& elem : elems) {
			const int count = winrt::VisualTreeHelper::GetChildrenCount(elem);
			for (int i = 0; i < count; ++i) {
				winrt::DependencyObject current = winrt::VisualTreeHelper::GetChild(elem, i);

				if (winrt::IInspectable tooltipContent = winrt::ToolTipService::GetToolTip(current)) {
					if (winrt::ToolTip tooltip = tooltipContent.try_as<winrt::ToolTip>()) {
						tooltip.RequestedTheme(theme);
					} else {
						winrt::ToolTip themedTooltip;
						themedTooltip.Content(tooltipContent);
						themedTooltip.RequestedTheme(theme);
						winrt::ToolTipService::SetToolTip(current, themedTooltip);
					}
				}

				temp.emplace_back(std::move(current));
			}
		}

		elems = std::move(temp);
	} while (!elems.empty());
}

bool XamlHelper::ContainsControl(const winrt::DependencyObject& parent, const winrt::DependencyObject& target) {
	std::vector<winrt::DependencyObject> elems{ parent };
	do {
		std::vector<winrt::DependencyObject> temp;

		for (const winrt::DependencyObject& elem : elems) {
			const int count = winrt::VisualTreeHelper::GetChildrenCount(elem);
			for (int i = 0; i < count; ++i) {
				winrt::DependencyObject current = winrt::VisualTreeHelper::GetChild(elem, i);

				if (current == target) {
					return true;
				}

				temp.emplace_back(std::move(current));
			}
		}

		elems = std::move(temp);
	} while (!elems.empty());

	return false;
}

bool XamlHelper::IsNullOrEmptyString(const winrt::IInspectable& value) noexcept {
	if (!value) {
		return true;
	}

	std::optional<winrt::hstring> str = value.try_as<winrt::hstring>();
	return str.has_value() && str->empty();
}

}
