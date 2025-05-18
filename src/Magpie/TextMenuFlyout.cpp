#include "pch.h"
#include "TextMenuFlyout.h"
#if __has_include("TextMenuFlyout.g.cpp")
#include "TextMenuFlyout.g.cpp"
#endif
#include "SmallVector.h"
#include "CommonSharedConstants.h"

using namespace ::Magpie;
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

	hstring selection;
	// 目前使用的 TextBox 和 NumberBox 都支持修改
	constexpr bool writable = true;

	// 没有通用的接口来获取选择的文本
	if (TextBox textBox = target.try_as<TextBox>()) {
		selection = textBox.SelectedText();
	} else if (MUXC::NumberBox numberBox = target.try_as<MUXC::NumberBox>()) {
		selection = numberBox.as<IControlProtected>()
			.GetTemplateChild(L"InputBox")
			.as<TextBox>()
			.SelectedText();
	}

	// 延迟初始化
	if (!_copy) {
		SmallVector<MenuFlyoutItemBase, 4> items;

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
		}
		items.emplace_back(_CreateMenuItem(
			Symbol{},
			resourceLoader.GetString(L"TextMenuFlyout_SelectAll"),
			{ this, &TextMenuFlyout::SelectAll_Click },
			VirtualKeyModifiers::Control,
			VirtualKey::A
		));

		Items().ReplaceAll({ items.data(), (uint32_t)items.size() });
	}

	const auto visibilityOfItemsRequiringSelection = selection.empty() ? Visibility::Collapsed : Visibility::Visible;
	if (_cut) {
		_cut.Visibility(visibilityOfItemsRequiringSelection);
	}
	_copy.Visibility(visibilityOfItemsRequiringSelection);
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
		numberBox.as<IControlProtected>().GetTemplateChild(L"InputBox").as<TextBox>().CutSelectionToClipboard();
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
		numberBox.as<IControlProtected>()
			.GetTemplateChild(L"InputBox")
			.as<TextBox>()
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
		numberBox.as<IControlProtected>()
			.GetTemplateChild(L"InputBox")
			.as<TextBox>()
			.PasteFromClipboard();
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
		numberBox.as<IControlProtected>()
			.GetTemplateChild(L"InputBox")
			.as<TextBox>()
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
	return item;
}

}
