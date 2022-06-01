#include "pch.h"
#include "SettingsGroup.h"
#if __has_include("SettingsGroup.g.cpp")
#include "SettingsGroup.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;


namespace winrt::Magpie::implementation {

const DependencyProperty SettingsGroup::ChildrenProperty = DependencyProperty::Register(
	L"Children",
	xaml_typename<UIElementCollection>(),
	xaml_typename<Magpie::SettingsGroup>(),
	PropertyMetadata(nullptr)
);

const DependencyProperty SettingsGroup::TitleProperty = DependencyProperty::Register(
	L"Title",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::SettingsGroup>(),
	PropertyMetadata(box_value(L""), &SettingsGroup::_OnPropertyChanged)
);

const DependencyProperty SettingsGroup::DescriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::SettingsGroup>(),
	PropertyMetadata(nullptr, &SettingsGroup::_OnPropertyChanged)
);

SettingsGroup::SettingsGroup() {
	InitializeComponent();

	Children(PART_Host().Children());
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

UIElementCollection SettingsGroup::Children() const {
	return GetValue(ChildrenProperty).as<UIElementCollection>();
}

void SettingsGroup::Children(UIElementCollection const& value) {
	SetValue(ChildrenProperty, value);
}

void SettingsGroup::SettingsGroup_IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&) {
	_SetEnabledState();
}

void SettingsGroup::SettingsGroup_Loading(FrameworkElement const&, IInspectable const&) {
	_SetEnabledState();
	_Update();
}

void SettingsGroup::_OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsGroup>(sender.as<default_interface<SettingsGroup>>())->_Update();
}

void SettingsGroup::_Update() {
	TitleTextBlock().Visibility(Title().empty() ? Visibility::Collapsed : Visibility::Visible);
	DescriptionPresenter().Visibility(Description() == nullptr ? Visibility::Collapsed : Visibility::Visible);
}

void SettingsGroup::_SetEnabledState() {
	VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", true);
}

}
