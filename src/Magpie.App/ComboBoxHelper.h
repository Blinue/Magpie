#pragma once
#include "XamlUtils.h"

namespace winrt::Magpie::App {

struct ComboBoxHelper {
	// 用于修复 ComboBox 中存在的问题
	// 因为官方毫无作为，我不得不使用这些 hack
	template<typename T>
	static void DropDownOpened(T const& page, IInspectable const& sender) {
		// 修复下拉框不适配主题的问题
		// https://github.com/microsoft/microsoft-ui-xaml/issues/6622
		XamlUtils::UpdateThemeOfXamlPopups(page.XamlRoot(), page.ActualTheme());

		// 修复下拉框位置不正确的问题
		// https://github.com/microsoft/microsoft-ui-xaml/issues/4551
		Controls::ComboBox comboBox = sender.as<Controls::ComboBox>();
		IInspectable selectedItem = comboBox.SelectedItem();
		if (!selectedItem) {
			return;
		}

		if (std::optional<hstring> str = selectedItem.try_as<hstring>()) {
			comboBox.PlaceholderText(*str);
		} else if (Controls::TextBlock textBlock = selectedItem.try_as<Controls::TextBlock>()) {
			comboBox.PlaceholderText(textBlock.Text());
		}
	}
};

}
