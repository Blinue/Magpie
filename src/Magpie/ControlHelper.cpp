#include "pch.h"
#include "ControlHelper.h"
#include "App.h"
#include "RootPage.h"

using namespace winrt;
using namespace winrt::Magpie::implementation;
using namespace Windows::UI::Xaml::Controls;

namespace Magpie {

void ControlHelper::ComboBox_DropDownOpened(const IInspectable& sender) {
	// 修复下拉框不适配主题的问题
	// https://github.com/microsoft/microsoft-ui-xaml/issues/6622
	const auto& rootPage = App::Get().RootPage();
	XamlHelper::UpdateThemeOfXamlPopups(rootPage->XamlRoot(), rootPage->ActualTheme());

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

void ControlHelper::NumberBox_Loaded(const IInspectable& sender) {
	// 确保模板已应用
	sender.as<MUXC::NumberBox>().ApplyTemplate();

	// 设置内部 TextBox 的右键菜单
	sender.as<IControlProtected>()
		.GetTemplateChild(L"InputBox")
		.as<TextBox>()
		.ContextFlyout(winrt::Magpie::TextMenuFlyout());
}

}
