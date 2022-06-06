#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "ShortcutDialogContent.g.h"


namespace winrt::Magpie::implementation {

struct ShortcutDialogContent : ShortcutDialogContentT<ShortcutDialogContent> {
	ShortcutDialogContent();

	void IsError(bool value);
	bool IsError() const;

	void Keys(const IVector<IInspectable>& value);
	IVector<IInspectable> Keys() const;

	static const DependencyProperty IsErrorProperty;
	static const DependencyProperty KeysProperty;
};

}

namespace winrt::Magpie::factory_implementation {

struct ShortcutDialogContent : ShortcutDialogContentT<ShortcutDialogContent, implementation::ShortcutDialogContent> {
};

}
