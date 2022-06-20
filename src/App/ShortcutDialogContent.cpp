#include "pch.h"
#include "ShortcutDialogContent.h"
#if __has_include("ShortcutDialogContent.g.cpp")
#include "ShortcutDialogContent.g.cpp"
#endif


using namespace winrt;


namespace winrt::Magpie::App::implementation {

const DependencyProperty ShortcutDialogContent::_IsErrorProperty = DependencyProperty::Register(
	L"_IsError",
	xaml_typename<bool>(),
	xaml_typename<Magpie::App::ShortcutDialogContent>(),
	PropertyMetadata(box_value(false), nullptr)
);

ShortcutDialogContent::ShortcutDialogContent() {
	InitializeComponent();
}

void ShortcutDialogContent::Error(HotkeyError value) {
	switch (value) {
	case HotkeyError::NoError:
		_IsError(false);
		WarningBanner().Visibility(Visibility::Collapsed);
		break;
	case HotkeyError::Invalid:
		_IsError(true);
		WarningBanner().Visibility(Visibility::Visible);
		InvalidShortcutWarningLabel().Text(L"无效快捷键");
		break;
	case HotkeyError::Occupied:
		_IsError(true);
		WarningBanner().Visibility(Visibility::Visible);
		InvalidShortcutWarningLabel().Text(L"此快捷键已被占用");
		break;
	default:
		break;
	}

	_error = value;
}

void ShortcutDialogContent::Keys(const IVector<IInspectable>& value) {
	_keys = value;
	KeysControl().ItemsSource(value);
}

IVector<IInspectable> ShortcutDialogContent::Keys() const {
	return _keys;
}

void ShortcutDialogContent::_IsError(bool value) {
	SetValue(_IsErrorProperty, box_value(value));
}

}
