#include "pch.h"
#include "ScalingModesPage.h"
#if __has_include("ScalingModesPage.g.cpp")
#include "ScalingModesPage.g.cpp"
#endif
#include "ComboBoxHelper.h"
#include "EffectsService.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;


namespace winrt::Magpie::App::implementation {

ScalingModesPage::ScalingModesPage() {
	InitializeComponent();

	auto items = AddEffectMenuFlyout().Items();
	for (const auto& effect : EffectsService::Get().Effects()) {
		MenuFlyoutItem item;
		item.Text(effect.name);
		items.Append(item);
	}
}

void ScalingModesPage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

void ScalingModesPage::AddEffectButton_Click(IInspectable const&, RoutedEventArgs const&) {
	AddEffectMenuFlyout().ShowAt(AddEffectButton());
}

}
