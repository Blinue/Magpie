#include "pch.h"
#include "ShortcutDialog.h"
#if __has_include("ShortcutDialog.g.cpp")
#include "ShortcutDialog.g.cpp"
#endif

using namespace winrt;
using namespace Windows::ApplicationModel::Resources;

namespace winrt::Magpie::App::implementation {

void ShortcutDialog::Error(ShortcutError value) {
	switch (value) {
	case ShortcutError::NoError:
	{
		WarningBanner().Visibility(Visibility::Collapsed);
		break;
	}
	case ShortcutError::Invalid:
	{
		WarningBanner().Visibility(Visibility::Visible);
		ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView();
		InvalidShortcutWarningLabel().Text(resourceLoader.GetString(L"ShortcutDialog_InvalidShortcut"));
		break;
	}
	case ShortcutError::Occupied:
	{
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

}
