#pragma once
#include "ShortcutDialog.g.h"
#include "ShortcutHelper.h"

namespace winrt::Magpie::implementation {

struct ShortcutDialog : ShortcutDialogT<ShortcutDialog> {
	void Error(::Magpie::ShortcutError value);
	void Keys(IVector<IInspectable> value);
};

}
