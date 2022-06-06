#include "pch.h"
#include "ShortcutDialogContent.h"
#if __has_include("ShortcutDialogContent.g.cpp")
#include "ShortcutDialogContent.g.cpp"
#endif


using namespace winrt;


namespace winrt::Magpie::implementation {

const DependencyProperty ShortcutDialogContent::IsErrorProperty = DependencyProperty::Register(
	L"IsError",
	xaml_typename<bool>(),
	xaml_typename<Magpie::ShortcutDialogContent>(),
	PropertyMetadata(box_value(false))
);

const DependencyProperty ShortcutDialogContent::KeysProperty = DependencyProperty::Register(
	L"Keys",
	xaml_typename<IVector<IInspectable>>(),
	xaml_typename<Magpie::ShortcutDialogContent>(),
	PropertyMetadata(nullptr)
);

ShortcutDialogContent::ShortcutDialogContent() {
	InitializeComponent();
}

void ShortcutDialogContent::IsError(bool value) {
	SetValue(IsErrorProperty, box_value(value));
}

bool ShortcutDialogContent::IsError() const {
	return GetValue(IsErrorProperty).as<bool>();
}

void ShortcutDialogContent::Keys(const IVector<IInspectable>& value) {
	SetValue(KeysProperty, value);
}

IVector<IInspectable> ShortcutDialogContent::Keys() const {
	return GetValue(KeysProperty).as<IVector<IInspectable>>();
}

}
