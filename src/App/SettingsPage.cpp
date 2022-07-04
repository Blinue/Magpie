#include "pch.h"
#include "SettingsPage.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif
#include "MainPage.h"
#include "XamlUtils.h"
#include "ComboBoxHelper.h"
#include "AppSettings.h"


using namespace winrt;
using namespace Windows::UI::Xaml::Input;


namespace winrt::Magpie::App::implementation {

SettingsPage::SettingsPage() {
	InitializeComponent();
}

void SettingsPage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

}
