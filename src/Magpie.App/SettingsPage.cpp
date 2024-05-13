#include "pch.h"
#include "SettingsPage.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif
#include "ComboBoxHelper.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Input;

namespace winrt::Magpie::App::implementation {

void SettingsPage::InitializeComponent() {
	SettingsPageT::InitializeComponent();

	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(L"Microsoft.UI.Xaml/Resources");
	hstring versionStr = resourceLoader.GetString(L"SettingsButtonName");
	SettingsPageFrame().Title(versionStr);
}

void SettingsPage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) const {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

}
