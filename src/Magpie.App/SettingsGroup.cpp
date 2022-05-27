#include "pch.h"
#include "SettingsGroup.h"
#if __has_include("SettingsGroup.g.cpp")
#include "SettingsGroup.g.cpp"
#endif
#if __has_include("SettingsGroupAutomationPeer.g.cpp")
#include "SettingsGroupAutomationPeer.g.cpp"
#endif

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Automation::Peers;


namespace winrt::Magpie::App::implementation {

const DependencyProperty SettingsGroup::MyHeaderProperty = DependencyProperty::Register(
	L"MyHeader",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::App::SettingsGroup>(),
	PropertyMetadata(box_value(L""), &SettingsGroup::_OnMyHeaderChanged)
);

const DependencyProperty SettingsGroup::DescriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::SettingsGroup>(),
	PropertyMetadata(nullptr, &SettingsGroup::_OnDescriptionChanged)
);

SettingsGroup::SettingsGroup() {
	DefaultStyleKey(box_value(name_of<Magpie::App::SettingsGroup>()));
}

void SettingsGroup::MyHeader(const hstring& value) {
	SetValue(MyHeaderProperty, box_value(value));
}

hstring SettingsGroup::MyHeader() const {
	return GetValue(MyHeaderProperty).as<hstring>();
}

void SettingsGroup::Description(IInspectable value) {
	SetValue(DescriptionProperty, value);
}

IInspectable SettingsGroup::Description() const {
	return GetValue(DescriptionProperty);
}

void SettingsGroup::OnApplyTemplate() {
	if (_isEnabledChangedToken) {
		IsEnabledChanged(_isEnabledChangedToken);
		_isEnabledChangedToken = {};
	}

	GetTemplateChild(_PartMyHeaderPresenter).as(_myHeaderPresenter);
	GetTemplateChild(_PartDescriptionPresenter).as(_descriptionPresenter);
	
	_SetEnabledState();
	_isEnabledChangedToken = IsEnabledChanged({ this, &SettingsGroup::_Setting_IsEnabledChanged });

	_Update();

	__super::OnApplyTemplate();
}

AutomationPeer SettingsGroup::OnCreateAutomationPeer() {
	return Magpie::App::SettingsGroupAutomationPeer(*this);
}

void SettingsGroup::_OnMyHeaderChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	winrt::get_self<SettingsGroup>(sender.as<default_interface<SettingsGroup>>())->_Update();
}

void SettingsGroup::_OnDescriptionChanged(DependencyObject const& sender,DependencyPropertyChangedEventArgs const&) {
	winrt::get_self<SettingsGroup>(sender.as<default_interface<SettingsGroup>>())->_Update();
}

void SettingsGroup::_Setting_IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&) {
	_SetEnabledState();
}

void SettingsGroup::_Update() {
	if (_myHeaderPresenter) {
		if (MyHeader().empty()) {
			_myHeaderPresenter.Visibility(Visibility::Collapsed);
		} else {
			_myHeaderPresenter.Visibility(Visibility::Visible);
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

void SettingsGroup::_SetEnabledState() {
	VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", true);
}

SettingsGroupAutomationPeer::SettingsGroupAutomationPeer(Magpie::App::SettingsGroup owner) : SettingsGroupAutomationPeerT<SettingsGroupAutomationPeer>(owner) {
}

hstring SettingsGroupAutomationPeer::GetNameCore() {
	return Owner().as<Magpie::App::SettingsGroup>().MyHeader();
}

}
