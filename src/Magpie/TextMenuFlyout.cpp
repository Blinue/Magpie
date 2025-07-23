#include "pch.h"
#include "TextMenuFlyout.h"
#if __has_include("TextMenuFlyout.g.cpp")
#include "TextMenuFlyout.g.cpp"
#endif
#include "CommonSharedConstants.h"

using namespace winrt::Windows::UI::Xaml::Input;

namespace winrt::Magpie::implementation {

TextMenuFlyout::TextMenuFlyout() {
	// 大部分初始化推迟到 MenuFlyout_Opening
	Opening({ this, &TextMenuFlyout::MenuFlyout_Opening });
}

void TextMenuFlyout::MenuFlyout_Opening(IInspectable const&, IInspectable const&) {
	FrameworkElement target = Target();
	if (!target) {
		return;
	}

	bool hasSelection = false;
	bool hasText = false;
	bool canUndo = false;
	bool canRedo = false;
	// 目前使用的 TextBox 和 NumberBox 都支持修改
	constexpr bool writable = true;

	{
		TextBox textBox = target.try_as<TextBox>();
		if (!textBox) {
			if (MUXC::NumberBox numberBox = target.try_as<MUXC::NumberBox>()) {
				textBox = numberBox.try_as<IControlProtected>()
					.GetTemplateChild(L"InputBox")
					.try_as<TextBox>();
			}
		}
		if (textBox) {
			hasSelection = !textBox.SelectedText().empty();
			hasText = !textBox.Text().empty();
			canUndo = textBox.CanUndo();
			canRedo = textBox.CanRedo();
		}
	}

	// 延迟初始化
	if (!_copy) {
		std::vector<MenuFlyoutItemBase> items;

		ResourceLoader resourceLoader =
			ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);

		if (writable) {
			_cut = items.emplace_back(_CreateMenuItem(
				Symbol::Cut,
				resourceLoader.GetString(L"TextMenuFlyout_Cut"),
				{ this, &TextMenuFlyout::Cut_Click },
				VirtualKeyModifiers::Control,
				VirtualKey::X
			));
		}
		_copy = items.emplace_back(_CreateMenuItem(
			Symbol::Copy,
			resourceLoader.GetString(L"TextMenuFlyout_Copy"),
			{ this, &TextMenuFlyout::Copy_Click },
			VirtualKeyModifiers::Control,
			VirtualKey::C
		));
		if (writable) {
			items.emplace_back(_CreateMenuItem(
				Symbol::Paste,
				resourceLoader.GetString(L"TextMenuFlyout_Paste"),
				{ this, &TextMenuFlyout::Paste_Click },
				VirtualKeyModifiers::Control,
				VirtualKey::V
			));
			_undo = _CreateMenuItem(
				Symbol::Undo,
				resourceLoader.GetString(L"TextMenuFlyout_Undo"),
				{ this, &TextMenuFlyout::Undo_Click },
				VirtualKeyModifiers::Control,
				VirtualKey::Z
			);
			items.emplace_back(_undo);
			_redo = _CreateMenuItem(
				Symbol::Redo,
				resourceLoader.GetString(L"TextMenuFlyout_Redo"),
				{ this, &TextMenuFlyout::Redo_Click },
				VirtualKeyModifiers::Control,
				VirtualKey::Y
			);
			items.emplace_back(_redo);
		}
		_selectAll = _CreateMenuItem(
			Symbol{},
			resourceLoader.GetString(L"TextMenuFlyout_SelectAll"),
			{ this, &TextMenuFlyout::SelectAll_Click },
			VirtualKeyModifiers::Control,
			VirtualKey::A
		);
		items.emplace_back(_selectAll);

		Items().ReplaceAll({ items.data(), (uint32_t)items.size() });
	}

	_copy.Visibility(hasSelection ? Visibility::Visible : Visibility::Collapsed);
	_selectAll.Visibility(hasText ? Visibility::Visible : Visibility::Collapsed);
	if (writable) {
		_cut.Visibility(hasSelection ? Visibility::Visible : Visibility::Collapsed);
		_undo.Visibility(canUndo ? Visibility::Visible : Visibility::Collapsed);
		_redo.Visibility(canRedo ? Visibility::Visible : Visibility::Collapsed);
	}
}

void TextMenuFlyout::Cut_Click(IInspectable const&, RoutedEventArgs const&) {
	// 右键菜单关闭后仍会接收到文本框上的快捷键事件，可以安全忽略，因为 TextBox
	// 仍会正常处理，除了 Ctrl+A
	FrameworkElement target = Target();
	if (!target) {
		return;
	}

	if (TextBox textBox = target.try_as<TextBox>()) {
		textBox.CutSelectionToClipboard();
	} else if (MUXC::NumberBox numberBox = target.try_as<MUXC::NumberBox>()) {
		numberBox.try_as<IControlProtected>().GetTemplateChild(L"InputBox").try_as<TextBox>().CutSelectionToClipboard();
	}
}

void TextMenuFlyout::Copy_Click(IInspectable const&, RoutedEventArgs const&) {
	FrameworkElement target = Target();
	if (!target) {
		return;
	}

	if (TextBox textBox = target.try_as<TextBox>()) {
		textBox.CopySelectionToClipboard();
	} else if (MUXC::NumberBox numberBox = target.try_as<MUXC::NumberBox>()) {
		numberBox.try_as<IControlProtected>()
			.GetTemplateChild(L"InputBox")
			.try_as<TextBox>()
			.CopySelectionToClipboard();
	}
}

void TextMenuFlyout::Paste_Click(IInspectable const&, RoutedEventArgs const&) {
	FrameworkElement target = Target();
	if (!target) {
		return;
	}

	if (TextBox textBox = target.try_as<TextBox>()) {
		textBox.PasteFromClipboard();
	} else if (MUXC::NumberBox numberBox = target.try_as<MUXC::NumberBox>()) {
		numberBox.try_as<IControlProtected>()
			.GetTemplateChild(L"InputBox")
			.try_as<TextBox>()
			.PasteFromClipboard();
	}
}

void TextMenuFlyout::Undo_Click(IInspectable const&, RoutedEventArgs const&) {
	FrameworkElement target = Target();
	if (!target) {
		return;
	}

	if (TextBox textBox = target.try_as<TextBox>()) {
		textBox.Undo();
	} else if (MUXC::NumberBox numberBox = target.try_as<MUXC::NumberBox>()) {
		numberBox.try_as<IControlProtected>()
			.GetTemplateChild(L"InputBox")
			.try_as<TextBox>()
			.Undo();
	}
}

void TextMenuFlyout::Redo_Click(IInspectable const&, RoutedEventArgs const&) {
	FrameworkElement target = Target();
	if (!target) {
		return;
	}

	if (TextBox textBox = target.try_as<TextBox>()) {
		textBox.Redo();
	} else if (MUXC::NumberBox numberBox = target.try_as<MUXC::NumberBox>()) {
		numberBox.try_as<IControlProtected>()
			.GetTemplateChild(L"InputBox")
			.try_as<TextBox>()
			.Redo();
	}
}

void TextMenuFlyout::SelectAll_Click(IInspectable const&, RoutedEventArgs const&) {
	// !!! HACK !!!
	// 由于 WinUI 的 bug，一旦右键菜单被打开过一次，TextBox 就不再处理 Ctrl+A，而我们
	// 仍会收到回调。这里必须自己实现全选功能，否则 Ctrl+A 会永久失效。
	IInspectable target = Target();
	if (!target) {
		target = FocusManager::GetFocusedElement(XamlRoot());
		if (!target) {
			return;
		}
	}

	if (TextBox textBox = target.try_as<TextBox>()) {
		textBox.SelectAll();
	} else if (MUXC::NumberBox numberBox = target.try_as<MUXC::NumberBox>()) {
		numberBox.try_as<IControlProtected>()
			.GetTemplateChild(L"InputBox")
			.try_as<TextBox>()
			.SelectAll();
	}
}

MenuFlyoutItemBase TextMenuFlyout::_CreateMenuItem(
	Symbol symbol,
	hstring text,
	RoutedEventHandler click,
	VirtualKeyModifiers modifiers,
	VirtualKey key
) {
	KeyboardAccelerator accel;
	accel.Modifiers(modifiers);
	accel.Key(key);

	MenuFlyoutItem item;
	if (symbol != Symbol{}) {
		item.Icon(SymbolIcon{ std::move(symbol) });
	}
	item.Text(std::move(text));
	item.Click(std::move(click));
	item.KeyboardAccelerators().Append(std::move(accel));

	// 存在快捷键时会有多余的 Tooltip，没有直接的删除方法，这里用不可见的 Tooltip 把它覆盖
	ToolTip tooltip;
	tooltip.Visibility(Visibility::Collapsed);
	ToolTipService::SetToolTip(item, tooltip);

	return item;
}

}
