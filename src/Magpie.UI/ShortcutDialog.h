#pragma once

#include "pch.h"
#include "ShortcutDialog.g.h"


namespace winrt::Magpie::UI::implementation {

struct ShortcutDialog : ShortcutDialogT<ShortcutDialog> {
	ShortcutDialog();

	void Error(HotkeyError value);
	HotkeyError Error() const {
		return _error;
	}

	void Keys(const IVector<IInspectable>& value);
	IVector<IInspectable> Keys() const;

private:
	static const DependencyProperty _IsErrorProperty;

	void _IsError(bool value);

	HotkeyError _error = HotkeyError::NoError;
	IVector<IInspectable> _keys = nullptr;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ShortcutDialog : ShortcutDialogT<ShortcutDialog, implementation::ShortcutDialog> {
};

}
