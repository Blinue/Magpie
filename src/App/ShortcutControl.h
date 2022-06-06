#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "ShortcutControl.g.h"
#include "HotkeySettings.h"
#include "ShortcutDialogContent.h"


namespace winrt::Magpie::implementation {

struct ShortcutControl : ShortcutControlT<ShortcutControl> {
	ShortcutControl();

	IAsyncAction EditButton_Click(IInspectable const&, RoutedEventArgs const&);

private:
	Magpie::HotkeySettings _hotkeySettings;
	Controls::ContentDialog _shortcutDialog;
	Magpie::ShortcutDialogContent _shortcutDialogContent;
};

}

namespace winrt::Magpie::factory_implementation {

struct ShortcutControl : ShortcutControlT<ShortcutControl, implementation::ShortcutControl> {
};

}
