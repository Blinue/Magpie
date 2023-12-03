#include "pch.h"
#include "SettingsGroup.h"
#if __has_include("SettingsGroup.g.cpp")
#include "SettingsGroup.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Data;

namespace winrt::Magpie::App::implementation {

const DependencyProperty SettingsGroup::ChildrenProperty = DependencyProperty::Register(
	L"Children",
	xaml_typename<UIElementCollection>(),
	xaml_typename<Magpie::App::SettingsGroup>(),
	nullptr
);

const DependencyProperty SettingsGroup::TitleProperty = DependencyProperty::Register(
	L"Title",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::App::SettingsGroup>(),
	PropertyMetadata(box_value(L""), &SettingsGroup::_OnTitleChanged)
);

const DependencyProperty SettingsGroup::DescriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::SettingsGroup>(),
	PropertyMetadata(nullptr, &SettingsGroup::_OnDescriptionChanged)
);

void SettingsGroup::InitializeComponent() {
	SettingsGroupT::InitializeComponent();

	Children(ChildrenHost().Children());
}

void SettingsGroup::IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&) {
	_SetEnabledState();
}

void SettingsGroup::Loading(FrameworkElement const&, IInspectable const&) {
	_SetEnabledState();
	_Update();
}

void SettingsGroup::_OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	SettingsGroup* that = get_self<SettingsGroup>(sender.as<default_interface<SettingsGroup>>());
	that->_Update();
	that->_propertyChangedEvent(*that, PropertyChangedEventArgs{ L"Title" });
}

void SettingsGroup::_OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	SettingsGroup* that = get_self<SettingsGroup>(sender.as<default_interface<SettingsGroup>>());
	that->_Update();
	that->_propertyChangedEvent(*that, PropertyChangedEventArgs{ L"Description" });
}

void SettingsGroup::_Update() {
	TitleTextBlock().Visibility(Title().empty() ? Visibility::Collapsed : Visibility::Visible);
	DescriptionPresenter().Visibility(Description() == nullptr ? Visibility::Collapsed : Visibility::Visible);
}

void SettingsGroup::_SetEnabledState() {
	VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", true);
}

}
