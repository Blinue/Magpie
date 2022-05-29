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
	DefaultStyleKey(box_value(name_of<Magpie::SettingsGroup>()));
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

void SettingsGroup::OnApplyTemplate() {
	if (_isEnabledChangedToken) {
		IsEnabledChanged(_isEnabledChangedToken);
		_isEnabledChangedToken = {};
	}

	GetTemplateChild(_PartTitlePresenter).as(_TitlePresenter);
	GetTemplateChild(_PartDescriptionPresenter).as(_descriptionPresenter);
	
	_SetEnabledState();
	_isEnabledChangedToken = IsEnabledChanged({ this, &SettingsGroup::_Setting_IsEnabledChanged });

	_Update();

	__super::OnApplyTemplate();
}

AutomationPeer SettingsGroup::OnCreateAutomationPeer() {
	return Magpie::SettingsGroupAutomationPeer(*this);
}

void SettingsGroup::_OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	winrt::get_self<SettingsGroup>(sender.as<default_interface<SettingsGroup>>())->_Update();
}

void SettingsGroup::_OnDescriptionChanged(DependencyObject const& sender,DependencyPropertyChangedEventArgs const&) {
	winrt::get_self<SettingsGroup>(sender.as<default_interface<SettingsGroup>>())->_Update();
}

void SettingsGroup::_Setting_IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&) {
	_SetEnabledState();
}

void SettingsGroup::_Update() {
	if (_TitlePresenter) {
		if (Title().empty()) {
			_TitlePresenter.Visibility(Visibility::Collapsed);
		} else {
			_TitlePresenter.Visibility(Visibility::Visible);
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

SettingsGroupAutomationPeer::SettingsGroupAutomationPeer(Magpie::SettingsGroup owner) : SettingsGroupAutomationPeerT<SettingsGroupAutomationPeer>(owner) {
}

hstring SettingsGroupAutomationPeer::GetNameCore() {
	return Owner().as<Magpie::SettingsGroup>().Title();
}

}
