#pragma once
#include "ShortcutDialog.g.h"

namespace winrt::Magpie::implementation {

struct ShortcutDialog : ShortcutDialogT<ShortcutDialog> {
	ShortcutError Error() const { return _error; }
	void Error(ShortcutError value);

	IVector<IInspectable> Keys() const { return _keys; }
	void Keys(IVector<IInspectable> value);

private:
	ShortcutError _error = ShortcutError::NoError;
	IVector<IInspectable> _keys{ nullptr };
};

}

namespace winrt::Magpie::factory_implementation {

struct ShortcutDialog : ShortcutDialogT<ShortcutDialog, implementation::ShortcutDialog> {
};

}
