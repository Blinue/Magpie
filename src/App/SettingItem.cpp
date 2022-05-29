#include "pch.h"
#include "SettingItem.h"
#if __has_include("SettingItem.g.cpp")
#include "SettingItem.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Automation;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Foundation;


namespace winrt::Magpie::implementation
{

DependencyProperty SettingItem::TitleProperty = DependencyProperty::Register(
	L"Title",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::SettingItem>(),
	PropertyMetadata(box_value(L""), &SettingItem::_OnTitleChanged)
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
	PropertyMetadata(box_value(L""), &SettingItem::_OnIconChanged)
);

DependencyProperty SettingItem::ActionContentProperty = DependencyProperty::Register(
	L"ActionContent",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::SettingItem>(),
	nullptr
);

SettingItem::SettingItem() {
	DefaultStyleKey(box_value(name_of<Magpie::SettingItem>()));
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

void SettingItem::OnApplyTemplate() {
	if (_isEnabledChangedToken) {
		IsEnabledChanged(_isEnabledChangedToken);
		_isEnabledChangedToken = {};
	}
	
	GetTemplateChild(_PartIconPresenter).as(_iconPresenter);
	GetTemplateChild(_PartDescriptionPresenter).as(_descriptionPresenter);
	
	_SetEnabledState();
	_isEnabledChangedToken = IsEnabledChanged({ this, &SettingItem::_Setting_IsEnabledChanged });

	_Update();

	__super::OnApplyTemplate();
}

void SettingItem::_Setting_IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&) {
	_SetEnabledState();
}

void SettingItem::_SetEnabledState() {
	VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", true);
}

void SettingItem::_OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	winrt::get_self<SettingItem>(sender.as<default_interface<SettingItem>>())->_Update();
}

void SettingItem::_OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	winrt::get_self<SettingItem>(sender.as<default_interface<SettingItem>>())->_Update();
}

void SettingItem::_OnIconChanged(DependencyObject const& sender,DependencyPropertyChangedEventArgs const&) {
	winrt::get_self<SettingItem>(sender.as<default_interface<SettingItem>>())->_Update();
}

void SettingItem::_Update() {
	if (ActionContent()) {
		if (winrt::get_class_name(ActionContent()) != name_of<Button>()) {
			// We do not want to override the default AutomationProperties.Name of a button. Its Content property already describes what it does.
			if (!Title().empty()) {
				AutomationProperties::SetName(ActionContent().as<UIElement>(), Title());
			}
		}
	}
	
	if (_iconPresenter) {
		if (Icon() == nullptr) {
			_iconPresenter.Visibility(Visibility::Collapsed);
		} else {
			_iconPresenter.Visibility(Visibility::Visible);
		}
	}

	if (_descriptionPresenter) {
		if (Description() == nullptr) {
			_descriptionPresenter.Visibility(Visibility::Collapsed);
		} else {
			_descriptionPresenter.Visibility(Visibility::Visible);
		}
	}
}
}
