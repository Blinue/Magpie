#include "pch.h"
#include "ShortcutDialogContent.h"
#if __has_include("ShortcutDialogContent.g.cpp")
#include "ShortcutDialogContent.g.cpp"
#endif
#include "LocalizationService.h"

using namespace ::Magpie;

namespace winrt::Magpie::implementation {

void ShortcutDialogContent::Error(ShortcutError value) {
	switch (value) {
	case ShortcutError::NoError:
	{
		WarningBanner().Visibility(Visibility::Collapsed);
		break;
	}
	case ShortcutError::Invalid:
	{
		WarningBanner().Visibility(Visibility::Visible);
		LocalizationService& ls = LocalizationService::Get();
		InvalidShortcutWarningLabel().Text(ls.GetLocalizedString(L"ShortcutDialog_InvalidShortcut"));
		break;
	}
	case ShortcutError::InUse:
	{
		WarningBanner().Visibility(Visibility::Visible);
		LocalizationService& ls = LocalizationService::Get();
		InvalidShortcutWarningLabel().Text(ls.GetLocalizedString(L"ShortcutDialog_InUse"));
		break;
	}
	default:
		assert(false);
		break;
	}
}

void ShortcutDialogContent::Keys(IVector<IInspectable> value) {
	KeysControl().ItemsSource(std::move(value));
}

}
