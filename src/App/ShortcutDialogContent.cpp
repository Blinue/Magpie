#include "pch.h"
#include "ShortcutDialogContent.h"
#if __has_include("ShortcutDialogContent.g.cpp")
#include "ShortcutDialogContent.g.cpp"
#endif


using namespace winrt;


namespace winrt::Magpie::implementation {

ShortcutDialogContent::ShortcutDialogContent() {
    InitializeComponent();
}

void ShortcutDialogContent::IsError(bool value) {
}

bool ShortcutDialogContent::IsError() const {
    return false;
}

void ShortcutDialogContent::Keys(const IVector<IInspectable>& value) {
}

IVector<IInspectable> ShortcutDialogContent::Keys() const {
    return IVector<IInspectable>();
}

}
