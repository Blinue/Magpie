#include "pch.h"
#include "TextMenuFlyout.h"
#if __has_include("TextMenuFlyout.g.cpp")
#include "TextMenuFlyout.g.cpp"
#endif
#include "SmallVector.h"

using namespace ::Magpie;
using namespace winrt;
using namespace Windows::UI::Xaml::Input;

namespace winrt::Magpie::implementation {

using MenuFlyoutItemClick = void (*)(IInspectable const&, RoutedEventArgs const&);

constexpr auto NullSymbol = static_cast<Symbol>(0);

TextMenuFlyout::TextMenuFlyout() {
	// Most of the initialization is delayed until the first call to MenuFlyout_Opening.
	Opening({ this, &TextMenuFlyout::MenuFlyout_Opening });
}

void TextMenuFlyout::MenuFlyout_Opening(IInspectable const&, IInspectable const&) {
	auto target = Target();
	if (!target) {
		return;
	}
	hstring selection;
	bool writable = false;

	// > Common group of selectable controls with common actions
	// > The I in MIDL stands for...
	// No common interface.
	if (MUXC::NumberBox numberBox = target.try_as<MUXC::NumberBox>()) {
		selection = numberBox.as<IControlProtected>()
			.GetTemplateChild(L"InputBox")
			.as<TextBox>()
			.SelectedText();
		writable = true;
	} else if (const auto textBlock = target.try_as<TextBlock>()) {
		selection = textBlock.SelectedText();
	} else if (const auto textBox = target.try_as<TextBox>()) {
		selection = textBox.SelectedText();
		writable = true;
	}

	if (!_copy) {
		SmallVector<MenuFlyoutItemBase, 4> items;

		if (writable) {
			_cut = items.emplace_back(_CreateMenuItem(Symbol::Cut, L"Cut", { this, &TextMenuFlyout::Cut_Click }, VirtualKeyModifiers::Control, VirtualKey::X));
		}
		_copy = items.emplace_back(_CreateMenuItem(Symbol::Copy, L"Copy", { this, &TextMenuFlyout::Copy_Click }, VirtualKeyModifiers::Control, VirtualKey::C));
		if (writable) {
			items.emplace_back(_CreateMenuItem(Symbol::Paste, L"Paste", { this, &TextMenuFlyout::Paste_Click }, VirtualKeyModifiers::Control, VirtualKey::V));
		}
		items.emplace_back(_CreateMenuItem(NullSymbol, L"SelectAll", { this, &TextMenuFlyout::SelectAll_Click }, VirtualKeyModifiers::Control, VirtualKey::A));

		Items().ReplaceAll({ items.data(), (uint32_t)items.size() });
	}

	const auto visibilityOfItemsRequiringSelection = selection.empty() ? Visibility::Collapsed : Visibility::Visible;
	if (_cut) {
		_cut.Visibility(visibilityOfItemsRequiringSelection);
	}
	_copy.Visibility(visibilityOfItemsRequiringSelection);
}

void TextMenuFlyout::Cut_Click(IInspectable const&, RoutedEventArgs const&) {
	// NOTE: When the flyout closes, WinUI doesn't disconnect the accelerator keys.
	// Since that means we'll get Ctrl+X/C/V callbacks forever, just ignore them.
	// The TextBox will still handle those events...
	auto target = Target();
	if (!target) {
		return;
	}

	if (const auto box = target.try_as<MUXC::NumberBox>()) {
		target = box.as<IControlProtected>().GetTemplateChild(L"InputBox").as<TextBox>();
	}
	if (const auto control = target.try_as<TextBox>()) {
		control.CutSelectionToClipboard();
	}
}

void TextMenuFlyout::Copy_Click(IInspectable const&, RoutedEventArgs const&) {
	auto target = Target();
	if (!target) {
		return;
	}

	if (MUXC::NumberBox numberBox = target.try_as<MUXC::NumberBox>()) {
		numberBox.as<IControlProtected>()
			.GetTemplateChild(L"InputBox")
			.as<TextBox>()
			.CopySelectionToClipboard();
	} else if (TextBlock textBlock = target.try_as<TextBlock>()) {
		textBlock.CopySelectionToClipboard();
	} else if (TextBox textBox = target.try_as<TextBox>()) {
		textBox.CopySelectionToClipboard();
	}
}

void TextMenuFlyout::Paste_Click(IInspectable const&, RoutedEventArgs const&) {
	auto target = Target();
	if (!target) {
		return;
	}

	if (MUXC::NumberBox numberBox = target.try_as<MUXC::NumberBox>()) {
		numberBox.as<IControlProtected>()
			.GetTemplateChild(L"InputBox")
			.as<TextBox>()
			.PasteFromClipboard();
	} else if (TextBox textBox = target.try_as<TextBox>()) {
		textBox.PasteFromClipboard();
	}
}

void TextMenuFlyout::SelectAll_Click(IInspectable const&, RoutedEventArgs const&) {
	// BODGY:
	// Once the flyout was open once, we'll get Ctrl+A events and the TextBox will
	// ignore them. As such, we have to dig out the focused element as a fallback,
	// because otherwise Ctrl+A will be permanently broken. Put differently,
	// this is bodgy because WinUI 2.8 is buggy. There's no other solution here.
	IInspectable target = Target();
	if (!target) {
		target = FocusManager::GetFocusedElement(XamlRoot());
		if (!target) {
			return;
		}
	}

	if (MUXC::NumberBox numberBox = target.try_as<MUXC::NumberBox>()) {
		numberBox.as<IControlProtected>()
			.GetTemplateChild(L"InputBox")
			.as<TextBox>()
			.SelectAll();
	} else if (TextBlock textBlock = target.try_as<TextBlock>()) {
		textBlock.SelectAll();
	} else if (TextBox textBox = target.try_as<TextBox>()) {
		textBox.SelectAll();
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
	if (symbol != NullSymbol) {
		item.Icon(SymbolIcon{ std::move(symbol) });
	}
	item.Text(std::move(text));
	item.Click(std::move(click));
	item.KeyboardAccelerators().Append(std::move(accel));
	return item;
}

}
