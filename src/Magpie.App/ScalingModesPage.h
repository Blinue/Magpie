#pragma once
#include "ScalingModesPage.g.h"
#include "ScalingType.g.h"

namespace winrt::Magpie::App::implementation {

struct ScalingType : ScalingTypeT<ScalingType> {
	ScalingType(const hstring& name, const hstring& desc) : _name(name), _desc(desc) {}

	hstring Name() const noexcept {
		return _name;
	}

	hstring Desc() const noexcept {
		return _desc;
	}

private:
	hstring _name;
	hstring _desc;
};

struct ScalingModesPage : ScalingModesPageT<ScalingModesPage> {
	ScalingModesPage();

	Magpie::App::ScalingModesViewModel ViewModel() const noexcept {
		return _viewModel;
	}

	static Windows::Globalization::NumberFormatting::INumberFormatter2 NumberFormatter() noexcept;

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

	void EffectSettingsCard_Loaded(IInspectable const& sender, RoutedEventArgs const&);

	void AddEffectButton_Click(IInspectable const& sender, RoutedEventArgs const&);

	void NewScalingModeFlyout_Opening(IInspectable const&, IInspectable const&);

	void NewScalingModeNameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args);

	void NewScalingModeConfirmButton_Click(IInspectable const& sender, RoutedEventArgs const&);

	void ScalingModeMoreOptionsButton_Click(IInspectable const& sender, RoutedEventArgs const&);

	void RemoveScalingModeMenuItem_Click(IInspectable const& sender, RoutedEventArgs const&);
private:
	void _BuildEffectMenu() noexcept;

	void _AddEffectMenuFlyoutItem_Click(IInspectable const& sender, RoutedEventArgs const&);

	IInspectable _moreOptionsButton{ nullptr };

	Controls::MenuFlyout _addEffectMenuFlyout;
	Magpie::App::ScalingModesViewModel _viewModel;
	Magpie::App::ScalingModeItem _curScalingMode{ nullptr };
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ScalingType : ScalingTypeT<ScalingType, implementation::ScalingType> {
};

struct ScalingModesPage : ScalingModesPageT<ScalingModesPage, implementation::ScalingModesPage> {
};

}
