#pragma once
#include "ScalingProfilePage.g.h"

namespace winrt::Magpie::UI::implementation {

struct ScalingProfilePage : ScalingProfilePageT<ScalingProfilePage> {
	ScalingProfilePage();

	void OnNavigatedTo(Navigation::NavigationEventArgs const& args);

	Magpie::UI::ScalingProfileViewModel ViewModel() const noexcept {
		return _viewModel;
	}

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

	void CursorScalingComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&);

	static Windows::Globalization::NumberFormatting::INumberFormatter2 NumberFormatter() noexcept;

	void RenameMenuItem_Click(IInspectable const&, RoutedEventArgs const&);

	void RenameFlyout_Opening(IInspectable const&, IInspectable const&);

	void RenameConfirmButton_Click(IInspectable const&, RoutedEventArgs const&);

	void RenameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args);

	void ReorderMenuItem_Click(IInspectable const&, RoutedEventArgs const&);

	void DeleteMenuItem_Click(IInspectable const&, RoutedEventArgs const&);

	void DeleteButton_Click(IInspectable const&, RoutedEventArgs const&);

private:
	Magpie::UI::ScalingProfileViewModel _viewModel{ nullptr };
	Windows::Globalization::NumberFormatting::DecimalFormatter _numberFormatter;

	MUXC::NavigationView::DisplayModeChanged_revoker _displayModeChangedRevoker{};
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingProfilePage : ScalingProfilePageT<ScalingProfilePage, implementation::ScalingProfilePage> {
};

}
