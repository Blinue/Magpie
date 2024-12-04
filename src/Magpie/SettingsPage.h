#pragma once
#include "SettingsPage.g.h"

namespace winrt::Magpie::implementation {

struct SettingsPage : SettingsPageT<SettingsPage> {
	void InitializeComponent();

	Magpie::SettingsViewModel ViewModel() const noexcept {
		return _viewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) const;

private:
	Magpie::SettingsViewModel _viewModel;
};

}

namespace winrt::Magpie::factory_implementation {

struct SettingsPage : SettingsPageT<SettingsPage, implementation::SettingsPage> {
};

}
