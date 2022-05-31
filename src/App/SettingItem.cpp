#include "pch.h"
#include "SettingItem.h"
#if __has_include("SettingItem.g.cpp")
#include "SettingItem.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;


namespace winrt::Magpie::implementation {

DependencyProperty SettingItem::TitleProperty = DependencyProperty::Register(
	L"Title",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::SettingItem>(),
	PropertyMetadata(box_value(L""), nullptr)
);

DependencyProperty SettingItem::DescriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::SettingItem>(),
	PropertyMetadata(nullptr, &SettingItem::_OnDescriptionChanged)
);

DependencyProperty SettingItem::IconProperty = DependencyProperty::Register(
	L"Icon",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::SettingItem>(),
	PropertyMetadata(nullptr, &SettingItem::_OnIconChanged)
);

DependencyProperty SettingItem::ActionContentProperty = DependencyProperty::Register(
	L"ActionContent",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::SettingItem>(),
	nullptr
);

SettingItem::SettingItem() {
	IsEnabledChanged({ this, &SettingItem::_IsEnabledChanged });

	InitializeComponent();
}

void SettingItem::Title(const hstring& value) {
	SetValue(TitleProperty, box_value(value));
}

hstring SettingItem::Title() const {
	return GetValue(TitleProperty).as<hstring>();
}

void SettingItem::Description(IInspectable value) {
	SetValue(DescriptionProperty, value);
}

IInspectable SettingItem::Description() const {
	return GetValue(DescriptionProperty);
}

void SettingItem::Icon(IInspectable value) {
	SetValue(IconProperty, value);
}

IInspectable SettingItem::Icon() const {
	return GetValue(IconProperty);
}

void SettingItem::ActionContent(IInspectable value) {
	SetValue(ActionContentProperty, value);
}

IInspectable SettingItem::ActionContent() const {
	return GetValue(ActionContentProperty);
}

void SettingItem::_OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args) {
	ContentPresenter desc = get_self<SettingItem>(sender.as<default_interface<SettingItem>>())->DescriptionPresenter();
	desc.Visibility(args.NewValue() == nullptr ? Visibility::Collapsed : Visibility::Visible);
}

void SettingItem::_OnIconChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args) {
	ContentPresenter icon = get_self<SettingItem>(sender.as<default_interface<SettingItem>>())->IconPresenter();
	icon.Visibility(args.NewValue() == nullptr ? Visibility::Collapsed : Visibility::Visible);
}

void SettingItem::_IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&) {
	VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", true);
}

}
