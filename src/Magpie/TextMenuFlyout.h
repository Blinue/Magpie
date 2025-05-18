#pragma once
#include "TextMenuFlyout.g.h"

namespace winrt::Magpie::implementation {

// GH#1070
// 移植自 https://github.com/microsoft/terminal/pull/18854
// 之所以使用自定义右键菜单，是因为当一个线程中创建了多个 XAML Islands 窗口，默认的
// 右键菜单会导致崩溃。
// 应确保覆盖所有右键菜单，目前包括 TextBox 和 MUXC::NumberBox，但不能在 App.xaml
// 中覆盖全局样式，否则仍会崩溃。因此我们对每个 TextBox 和 NumberBox 都单独设置。
struct TextMenuFlyout : TextMenuFlyoutT<TextMenuFlyout> {
	TextMenuFlyout();

	void MenuFlyout_Opening(IInspectable const&, IInspectable const&);
	void Cut_Click(IInspectable const&, RoutedEventArgs const&);
	void Copy_Click(IInspectable const&, RoutedEventArgs const&);
	void Paste_Click(IInspectable const&, RoutedEventArgs const&);
	void SelectAll_Click(IInspectable const&, RoutedEventArgs const&);

private:
	MenuFlyoutItemBase _CreateMenuItem(
		Symbol symbol,
		hstring text,
		RoutedEventHandler click,
		VirtualKeyModifiers modifiers,
		VirtualKey key
	);

	// 始终显示的条目
	MenuFlyoutItemBase _copy{ nullptr };
	// 只适用于可编辑的控件的条目
	MenuFlyoutItemBase _cut{ nullptr };
};

}

BASIC_FACTORY(TextMenuFlyout)
