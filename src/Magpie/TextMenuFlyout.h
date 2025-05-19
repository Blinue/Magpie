#pragma once
#include "TextMenuFlyout.g.h"

namespace winrt::Magpie::implementation {

// GH#1070
// 
// 移植自 https://github.com/microsoft/terminal/pull/18854
// 
// 之所以使用自定义右键菜单，是因为当一个线程中创建了多个 XAML Islands 窗口，默认的
// 右键菜单会导致崩溃。应确保覆盖所有右键菜单，目前包括 TextBox 和 MUXC::NumberBox。
// 
// 我还尝试过其他方案（如 MUXC::TextCommandBarFlyout），但都不尽人意。目前看来对每个
// TextBox 和 NumberBox 单独设置 TextMenuFlyout 是最稳妥的方案。
struct TextMenuFlyout : TextMenuFlyoutT<TextMenuFlyout> {
	TextMenuFlyout();

	void MenuFlyout_Opening(IInspectable const&, IInspectable const&);
	void Cut_Click(IInspectable const&, RoutedEventArgs const&);
	void Copy_Click(IInspectable const&, RoutedEventArgs const&);
	void Paste_Click(IInspectable const&, RoutedEventArgs const&);
	void Undo_Click(IInspectable const&, RoutedEventArgs const&);
	void Redo_Click(IInspectable const&, RoutedEventArgs const&);
	void SelectAll_Click(IInspectable const&, RoutedEventArgs const&);

private:
	MenuFlyoutItemBase _CreateMenuItem(
		Symbol symbol,
		hstring text,
		RoutedEventHandler click,
		VirtualKeyModifiers modifiers,
		VirtualKey key
	);

	// 始终存在的条目
	MenuFlyoutItemBase _copy{ nullptr };
	MenuFlyoutItemBase _selectAll{ nullptr };
	// 只适用于可编辑的控件的条目
	MenuFlyoutItemBase _cut{ nullptr };
	MenuFlyoutItemBase _undo{ nullptr };
	MenuFlyoutItemBase _redo{ nullptr };
};

}

BASIC_FACTORY(TextMenuFlyout)
