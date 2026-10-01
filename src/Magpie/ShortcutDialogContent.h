#pragma once
#include "ShortcutDialogContent.g.h"
#include "ShortcutHelper.h"

namespace winrt::Magpie::implementation {

struct ShortcutDialogContent : ShortcutDialogContentT<ShortcutDialogContent> {
	void Error(::Magpie::ShortcutError value);
	void Keys(IVector<IInspectable> value);
};

}
