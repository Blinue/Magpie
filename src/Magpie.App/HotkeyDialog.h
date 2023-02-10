#pragma once
#include "HotkeyDialog.g.h"

namespace winrt::Magpie::App::implementation {

struct HotkeyDialog : HotkeyDialogT<HotkeyDialog> {
	HotkeyDialog();

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
	IVector<IInspectable> _keys{ nullptr };
};

}

namespace winrt::Magpie::App::factory_implementation {

struct HotkeyDialog : HotkeyDialogT<HotkeyDialog, implementation::HotkeyDialog> {
};

}
