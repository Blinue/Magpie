#include "pch.h"
#include "ShortcutDialog.h"
#if __has_include("ShortcutDialog.g.cpp")
#include "ShortcutDialog.g.cpp"
#endif


namespace winrt::Magpie::UI::implementation {

const DependencyProperty ShortcutDialog::_IsErrorProperty = DependencyProperty::Register(
	L"_IsError",
	xaml_typename<bool>(),
	xaml_typename<Magpie::UI::ShortcutDialog>(),
	PropertyMetadata(box_value(false), nullptr)
);

ShortcutDialog::ShortcutDialog() {
	InitializeComponent();
}

void ShortcutDialog::Error(HotkeyError value) {
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

void ShortcutDialog::Keys(const IVector<IInspectable>& value) {
	_keys = value;
	KeysControl().ItemsSource(value);
}

IVector<IInspectable> ShortcutDialog::Keys() const {
	return _keys;
}

void ShortcutDialog::_IsError(bool value) {
	SetValue(_IsErrorProperty, box_value(value));
}

}
