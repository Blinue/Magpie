#pragma once
#include "ProfilePage.g.h"
#include "ProfileViewModel.h"

namespace winrt::Magpie::implementation {

struct ProfilePage : ProfilePageT<ProfilePage> {
	void InitializeComponent();

	void OnNavigatedTo(Navigation::NavigationEventArgs const& args);

	winrt::Magpie::ProfileViewModel ViewModel() const noexcept {
		return *_viewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

	void CursorScalingComboBox_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&);

	static Windows::Globalization::NumberFormatting::INumberFormatter2 NumberFormatter() noexcept;

	void RenameMenuItem_Click(IInspectable const&, RoutedEventArgs const&);

	void RenameFlyout_Opening(IInspectable const&, IInspectable const&);

	void RenameConfirmButton_Click(IInspectable const&, RoutedEventArgs const&);

	void RenameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args);

	void ReorderMenuItem_Click(IInspectable const&, RoutedEventArgs const&);

	void DeleteMenuItem_Click(IInspectable const&, RoutedEventArgs const&);

	void DeleteButton_Click(IInspectable const&, RoutedEventArgs const&);

	void LaunchParametersTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args);

private:
	com_ptr<ProfileViewModel> _viewModel;
};

}

namespace winrt::Magpie::factory_implementation {

struct ProfilePage : ProfilePageT<ProfilePage, implementation::ProfilePage> {
};

}
