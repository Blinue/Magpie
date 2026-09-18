#include "pch.h"
#include "ControlHelper.h"
#include "App.h"
#include "RootPage.h"

using namespace winrt::Magpie::implementation;
namespace winrt {
using namespace Windows::UI::Xaml::Controls;
}

namespace Magpie {

void ControlHelper::ComboBox_DropDownOpened(const IInspectable& sender) {
	// 修复下拉框不适配主题的问题
	// https://github.com/microsoft/microsoft-ui-xaml/issues/6622
	const auto& rootPage = App::Get().RootPage();
	XamlHelper::UpdateThemeOfXamlPopups(rootPage.XamlRoot(), rootPage.ActualTheme());

	// 修复下拉框位置不正确的问题
	// https://github.com/microsoft/microsoft-ui-xaml/issues/4551
	winrt::ComboBox comboBox = sender.try_as<winrt::ComboBox>();
	winrt::IInspectable selectedItem = comboBox.SelectedItem();
	if (!selectedItem) {
		return;
	}

	if (std::optional<winrt::hstring> str = selectedItem.try_as<winrt::hstring>()) {
		comboBox.PlaceholderText(*str);
	} else if (winrt::ContentControl container = selectedItem.try_as<winrt::ContentControl>()) {
		if (std::optional<winrt::hstring> strContent = container.Content().try_as<winrt::hstring>()) {
			comboBox.PlaceholderText(*strContent);
		}
	}
}

void ControlHelper::NumberBox_Loaded(const IInspectable& sender) {
	// 确保模板已应用
	sender.try_as<winrt::MUXC::NumberBox>().ApplyTemplate();

	// 设置内部 TextBox 的右键菜单
	sender.try_as<winrt::IControlProtected>()
		.GetTemplateChild(L"InputBox")
		.try_as<winrt::TextBox>()
		.ContextFlyout(winrt::Magpie::TextMenuFlyout());
}

}
