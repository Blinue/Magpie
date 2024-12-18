#include "pch.h"
#include "ShortcutDialog.h"
#if __has_include("ShortcutDialog.g.cpp")
#include "ShortcutDialog.g.cpp"
#endif
#include "CommonSharedConstants.h"

using namespace ::Magpie;

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
	case ShortcutError::InUse:
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
}

void ShortcutDialog::Keys(IVector<IInspectable> value) {
	KeysControl().ItemsSource(std::move(value));
}

}
