#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "ShortcutControl.g.h"
#include "HotkeySettings.h"
#include "ShortcutDialogContent.h"


namespace winrt::Magpie::App::implementation {

struct ShortcutControl : ShortcutControlT<ShortcutControl> {
	ShortcutControl();

	IAsyncAction EditButton_Click(IInspectable const&, RoutedEventArgs const&);

	void ShortcutDialog_Opened(Controls::ContentDialog const&, Controls::ContentDialogOpenedEventArgs const&);
	void ShortcutDialog_Closing(Controls::ContentDialog const&, Controls::ContentDialogClosingEventArgs const&);

private:
	static LRESULT CALLBACK _LowLevelKeyboardProc(
		int    nCode,
		WPARAM wParam,
		LPARAM lParam
	);

	Magpie::App::HotkeySettings _hotkey;
	Controls::ContentDialog _shortcutDialog;
	Magpie::App::ShortcutDialogContent _shortcutDialogContent;

	HHOOK _keyboardHook = NULL;
	static ShortcutControl* _that;

	Magpie::App::HotkeySettings _previewHotkey;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ShortcutControl : ShortcutControlT<ShortcutControl, implementation::ShortcutControl> {
};

}
