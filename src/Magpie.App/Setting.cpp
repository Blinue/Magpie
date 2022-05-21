#include "pch.h"
#include "Setting.h"
#if __has_include("Controls.Setting.g.cpp")
#include "Controls.Setting.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Automation;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Foundation;


namespace winrt::Magpie::App::Controls::implementation
{

DependencyProperty Setting::MyHeaderProperty = DependencyProperty::Register(
	L"MyHeader",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::App::Controls::Setting>(),
	PropertyMetadata(box_value(L""), &Setting::_OnMyHeaderChanged)
);

DependencyProperty Setting::DescriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::Controls::Setting>(),
	PropertyMetadata(nullptr, &Setting::_OnDescriptionChanged)
);

DependencyProperty Setting::IconProperty = DependencyProperty::Register(
	L"Icon",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::Controls::Setting>(),
	PropertyMetadata(box_value(L""), &Setting::_OnIconChanged)
);

DependencyProperty Setting::ActionContentProperty = DependencyProperty::Register(
	L"ActionContent",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::Controls::Setting>(),
	nullptr
);

Setting::Setting() {
	DefaultStyleKey(box_value(name_of<Magpie::App::Controls::Setting>()));
}

void Setting::MyHeader(const hstring& value) {
	SetValue(MyHeaderProperty, box_value(value));
}

hstring Setting::MyHeader() const {
	return GetValue(MyHeaderProperty).as<hstring>();
}

void Setting::Description(IInspectable value) {
	SetValue(DescriptionProperty, value);
}

IInspectable Setting::Description() const {
	return GetValue(DescriptionProperty);
}

void Setting::Icon(IInspectable value) {
	SetValue(IconProperty, value);
}

IInspectable Setting::Icon() const {
	return GetValue(IconProperty);
}

void Setting::ActionContent(IInspectable value) {
	SetValue(ActionContentProperty, value);
}

IInspectable Setting::ActionContent() const {
	return GetValue(ActionContentProperty);
}

void Setting::OnApplyTemplate() {
	if (_isEnabledChangedToken) {
		IsEnabledChanged(_isEnabledChangedToken);
		_isEnabledChangedToken = {};
	}
	
	_setting = this;
	GetTemplateChild(_PartIconPresenter).as(_iconPresenter);
	GetTemplateChild(_PartDescriptionPresenter).as(_descriptionPresenter);
	
	_SetEnabledState();
	_isEnabledChangedToken = IsEnabledChanged({ this, &Setting::_Setting_IsEnabledChanged });

	_Update();

	__super::OnApplyTemplate();
}

void Setting::_Setting_IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&) {
	_SetEnabledState();
}

void Setting::_SetEnabledState() {
	VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", true);
}

void Setting::_OnMyHeaderChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	winrt::get_self<Setting>(sender.as<default_interface<Setting>>())->_Update();
}

void Setting::_OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	winrt::get_self<Setting>(sender.as<default_interface<Setting>>())->_Update();
}

void Setting::_OnIconChanged(DependencyObject const& sender,DependencyPropertyChangedEventArgs const&) {
	winrt::get_self<Setting>(sender.as<default_interface<Setting>>())->_Update();
}

void Setting::_Update() {
	if (_setting == nullptr) {
		return;
	}
	
	if (ActionContent() != nullptr) {
		if (winrt::get_class_name(ActionContent()) != name_of<Button>()) {
			// We do not want to override the default AutomationProperties.Name of a button. Its Content property already describes what it does.
			if (!MyHeader().empty()) {
				AutomationProperties::SetName(ActionContent().as<UIElement>(), MyHeader());
			}
		}
	}
	
	if (_iconPresenter != nullptr) {
		if (Icon() == nullptr) {
			_iconPresenter.Visibility(Visibility::Collapsed);
		} else {
			_iconPresenter.Visibility(Visibility::Visible);
		}
	}

	if (Description() == nullptr) {
		_descriptionPresenter.Visibility(Visibility::Collapsed);
	} else {
		_descriptionPresenter.Visibility(Visibility::Visible);
	}
}
}
