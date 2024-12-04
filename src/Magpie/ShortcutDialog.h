#pragma once
#include "ShortcutDialog.g.h"

namespace winrt::Magpie::implementation {

struct ShortcutDialog : ShortcutDialogT<ShortcutDialog> {
	void Error(ShortcutError value);
	ShortcutError Error() const {
		return _error;
	}

	void Keys(const IVector<IInspectable>& value);
	IVector<IInspectable> Keys() const;

private:
	ShortcutError _error = ShortcutError::NoError;
	IVector<IInspectable> _keys{ nullptr };
};

}

namespace winrt::Magpie::factory_implementation {

struct ShortcutDialog : ShortcutDialogT<ShortcutDialog, implementation::ShortcutDialog> {
};

}
