#pragma once
#include "TextMenuFlyout.g.h"

namespace winrt::Magpie::implementation {

// GH#1070
// This custom flyout exists because WinUI 2 only supports 1 text block flyout
// *per thread* not per window. If you have >1 window per 1 thread, as we do,
// the focus will just be delegated to the window the flyout was first opened in.
// Once the first window is gone, WinUI will either do nothing or crash.
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

	// These are always present.
	MenuFlyoutItemBase _copy{ nullptr };
	// These are only set for writable controls.
	MenuFlyoutItemBase _cut{ nullptr };
};

}

BASIC_FACTORY(TextMenuFlyout)
