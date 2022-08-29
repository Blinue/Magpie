#pragma once
#include "SettingsPage.g.h"


namespace winrt::Magpie::UI::implementation {

struct SettingsPage : SettingsPageT<SettingsPage> {
	SettingsPage();
	
	Magpie::UI::SettingsViewModel ViewModel() const noexcept {
		return _viewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

private:
	Magpie::UI::SettingsViewModel _viewModel;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct SettingsPage : SettingsPageT<SettingsPage, implementation::SettingsPage> {
};

}
