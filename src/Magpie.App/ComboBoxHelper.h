#pragma once
#include "XamlUtils.h"

namespace winrt::Magpie::App {

struct ComboBoxHelper {
	// 用于修复 ComboBox 中存在的问题
	// 因为官方毫无作为，我不得不使用这些 hack
	template<typename T>
	static void DropDownOpened(T const& page, IInspectable const& sender) {
		using namespace Windows::UI::Xaml::Controls;

		// 修复下拉框不适配主题的问题
		// https://github.com/microsoft/microsoft-ui-xaml/issues/6622
		XamlUtils::UpdateThemeOfXamlPopups(page.XamlRoot(), page.ActualTheme());

		// 修复下拉框位置不正确的问题
		// https://github.com/microsoft/microsoft-ui-xaml/issues/4551
		ComboBox comboBox = sender.as<ComboBox>();
		IInspectable selectedItem = comboBox.SelectedItem();
		if (!selectedItem) {
			return;
		}

		if (std::optional<hstring> str = selectedItem.try_as<hstring>()) {
			comboBox.PlaceholderText(*str);
		} else if (ContentControl container = selectedItem.try_as<ContentControl>()) {
			if (std::optional<hstring> strContent = container.Content().try_as<hstring>()) {
				comboBox.PlaceholderText(*strContent);
			}
		}
	}
};

}
