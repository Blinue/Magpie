#include "pch.h"
#include "SettingsGroup.h"
#if __has_include("SettingsGroup.g.cpp")
#include "SettingsGroup.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Data;

namespace winrt::Magpie::App::implementation {

const DependencyProperty SettingsGroup::_headerProperty = DependencyProperty::Register(
	L"Header",
	xaml_typename<IInspectable>(),
	xaml_typename<class_type>(),
	PropertyMetadata(nullptr, &SettingsGroup::_OnHeaderChanged)
);

const DependencyProperty SettingsGroup::_descriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<class_type>(),
	PropertyMetadata(nullptr, &SettingsGroup::_OnDescriptionChanged)
);

void SettingsGroup::IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&) {
	_SetEnabledState();
}

void SettingsGroup::Loading(FrameworkElement const&, IInspectable const&) {
	_SetEnabledState();
	_Update();
}

void SettingsGroup::_OnHeaderChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	SettingsGroup* that = get_self<SettingsGroup>(sender.as<class_type>());
	that->_Update();
	that->_propertyChangedEvent(*that, PropertyChangedEventArgs{ L"Header" });
}

void SettingsGroup::_OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	SettingsGroup* that = get_self<SettingsGroup>(sender.as<class_type>());
	that->_Update();
	that->_propertyChangedEvent(*that, PropertyChangedEventArgs{ L"Description" });
}

void SettingsGroup::_Update() {
	HeaderTextBlock().Visibility(Header() == nullptr ? Visibility::Collapsed : Visibility::Visible);
	DescriptionPresenter().Visibility(Description() == nullptr ? Visibility::Collapsed : Visibility::Visible);
}

void SettingsGroup::_SetEnabledState() {
	VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", true);
}

}
