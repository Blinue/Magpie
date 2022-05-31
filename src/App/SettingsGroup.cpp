#include "pch.h"
#include "SettingsGroup.h"
#if __has_include("SettingsGroup.g.cpp")
#include "SettingsGroup.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;


namespace winrt::Magpie::implementation {

const DependencyProperty SettingsGroup::TitleProperty = DependencyProperty::Register(
	L"Title",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::SettingsGroup>(),
	PropertyMetadata(box_value(L""), &SettingsGroup::_OnTitleChanged)
);

const DependencyProperty SettingsGroup::DescriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::SettingsGroup>(),
	PropertyMetadata(nullptr, &SettingsGroup::_OnDescriptionChanged)
);

SettingsGroup::SettingsGroup() {
	InitializeComponent();

	IsEnabledChanged({ this, &SettingsGroup::_IsEnabledChanged });
}

void SettingsGroup::Title(const hstring& value) {
	SetValue(TitleProperty, box_value(value));
}

hstring SettingsGroup::Title() const {
	return GetValue(TitleProperty).as<hstring>();
}

void SettingsGroup::Description(IInspectable value) {
	SetValue(DescriptionProperty, value);
}

IInspectable SettingsGroup::Description() const {
	return GetValue(DescriptionProperty);
}

void SettingsGroup::_OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args) {
	TextBlock titleTextBlock = get_self<SettingsGroup>(sender.as<default_interface<SettingsGroup>>())->TitleTextBlock();
	titleTextBlock.Visibility(unbox_value<hstring>(args.NewValue()).empty() ? Visibility::Collapsed : Visibility::Visible);
}

void SettingsGroup::_OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args) {
	ContentPresenter desc = get_self<SettingsGroup>(sender.as<default_interface<SettingsGroup>>())->DescriptionPresenter();
	desc.Visibility(args.NewValue() == nullptr ? Visibility::Collapsed : Visibility::Visible);
}

void SettingsGroup::_IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&) {
	VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", true);
}

}
