#include "pch.h"
#include "ShortcutDialog.h"
#if __has_include("ShortcutDialog.g.cpp")
#include "ShortcutDialog.g.cpp"
#endif

using namespace winrt;
using namespace Windows::ApplicationModel::Resources;

namespace winrt::Magpie::App::implementation {

const DependencyProperty ShortcutDialog::_IsErrorProperty = DependencyProperty::Register(
	L"_IsError",
	xaml_typename<bool>(),
	xaml_typename<Magpie::App::ShortcutDialog>(),
	PropertyMetadata(box_value(false), nullptr)
);

ShortcutDialog::ShortcutDialog() {
	InitializeComponent();
}

void ShortcutDialog::Error(ShortcutError value) {
	switch (value) {
	case ShortcutError::NoError:
	{
		_IsError(false);
		WarningBanner().Visibility(Visibility::Collapsed);
		break;
	}
	case ShortcutError::Invalid:
	{
		_IsError(true);
		WarningBanner().Visibility(Visibility::Visible);
		ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView();
		InvalidShortcutWarningLabel().Text(resourceLoader.GetString(L"ShortcutDialog_InvalidShortcut"));
		break;
	}
	case ShortcutError::Occupied:
	{
		_IsError(true);
		WarningBanner().Visibility(Visibility::Visible);
		ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView();
		InvalidShortcutWarningLabel().Text(resourceLoader.GetString(L"ShortcutDialog_InUse"));
		break;
	}
	default:
		assert(false);
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
