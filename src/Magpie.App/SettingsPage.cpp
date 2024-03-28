#include "pch.h"
#include "SettingsPage.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif
#include "XamlUtils.h"
#include "ComboBoxHelper.h"
#include "CommonSharedConstants.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Input;

namespace winrt::Magpie::App::implementation {

void SettingsPage::InitializeComponent() {
	SettingsPageT::InitializeComponent();

	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	hstring versionStr = resourceLoader.
		GetString(L"ms-resource://Magpie.App/Microsoft.UI.Xaml/Resources/SettingsButtonName");
	SettingsPageFrame().Title(versionStr);
}

void SettingsPage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

}
