#pragma once
#include "SettingsPage.g.h"
#include "SettingsViewModel.h"

namespace winrt::Magpie::implementation {

struct SettingsPage : SettingsPageT<SettingsPage> {
	void InitializeComponent();

	winrt::Magpie::SettingsViewModel ViewModel() const noexcept {
		return *_viewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) const;

private:
	com_ptr<SettingsViewModel> _viewModel = make_self<SettingsViewModel>();
};

}

BASIC_FACTORY(SettingsPage)
