#include "pch.h"
#include "HotkeyDialog.h"
#if __has_include("HotkeyDialog.g.cpp")
#include "HotkeyDialog.g.cpp"
#endif

namespace winrt::Magpie::App::implementation {

const DependencyProperty HotkeyDialog::_IsErrorProperty = DependencyProperty::Register(
	L"_IsError",
	xaml_typename<bool>(),
	xaml_typename<Magpie::App::HotkeyDialog>(),
	PropertyMetadata(box_value(false), nullptr)
);

HotkeyDialog::HotkeyDialog() {
	InitializeComponent();
}

void HotkeyDialog::Error(HotkeyError value) {
	switch (value) {
	case HotkeyError::NoError:
		_IsError(false);
		WarningBanner().Visibility(Visibility::Collapsed);
		break;
	case HotkeyError::Invalid:
		_IsError(true);
		WarningBanner().Visibility(Visibility::Visible);
		InvalidHotkeyWarningLabel().Text(L"无效快捷键");
		break;
	case HotkeyError::Occupied:
		_IsError(true);
		WarningBanner().Visibility(Visibility::Visible);
		InvalidHotkeyWarningLabel().Text(L"此快捷键已被占用");
		break;
	default:
		break;
	}

	_error = value;
}

void HotkeyDialog::Keys(const IVector<IInspectable>& value) {
	_keys = value;
	KeysControl().ItemsSource(value);
}

IVector<IInspectable> HotkeyDialog::Keys() const {
	return _keys;
}

void HotkeyDialog::_IsError(bool value) {
	SetValue(_IsErrorProperty, box_value(value));
}

}
