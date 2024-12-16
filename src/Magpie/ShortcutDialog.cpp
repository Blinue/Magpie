#include "pch.h"
#include "ShortcutDialog.h"
#if __has_include("ShortcutDialog.g.cpp")
#include "ShortcutDialog.g.cpp"
#endif
#include "CommonSharedConstants.h"

namespace winrt::Magpie::implementation {

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
		ResourceLoader resourceLoader =
			ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
		InvalidShortcutWarningLabel().Text(resourceLoader.GetString(L"ShortcutDialog_InvalidShortcut"));
		break;
	}
	case ShortcutError::Occupied:
	{
		WarningBanner().Visibility(Visibility::Visible);
		ResourceLoader resourceLoader =
			ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
		InvalidShortcutWarningLabel().Text(resourceLoader.GetString(L"ShortcutDialog_InUse"));
		break;
	}
	default:
		assert(false);
		break;
	}

	_error = value;
}

void ShortcutDialog::Keys(IVector<IInspectable> value) {
	_keys = std::move(value);
	KeysControl().ItemsSource(_keys);
}

}
