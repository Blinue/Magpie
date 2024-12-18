#pragma once
#include "ScalingModesPage.g.h"
#include "ScalingType.g.h"
#include "ScalingModeItem.h"
#include "ScalingModesViewModel.h"

namespace winrt::Magpie::implementation {

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

	winrt::Magpie::ScalingModesViewModel ViewModel() const noexcept {
		return *_viewModel;
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
	com_ptr<ScalingModesViewModel> _viewModel = make_self<ScalingModesViewModel>();
	ScalingModeItem* _curScalingMode = nullptr;
};

}

namespace winrt::Magpie::factory_implementation {

struct ScalingType : ScalingTypeT<ScalingType, implementation::ScalingType> {
};

struct ScalingModesPage : ScalingModesPageT<ScalingModesPage, implementation::ScalingModesPage> {
};

}
