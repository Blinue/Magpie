#include "pch.h"
#include "SettingsCard.h"
#if __has_include("SettingsCard.g.cpp")
#include "SettingsCard.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Data;

namespace winrt::Magpie::App::implementation {

DependencyProperty SettingsCard::RawTitleProperty = DependencyProperty::Register(
	L"RawTitle",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::SettingsCard>(),
	PropertyMetadata(nullptr, _OnTitleChanged)
);

DependencyProperty SettingsCard::TitleProperty = DependencyProperty::Register(
	L"Title",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::App::SettingsCard>(),
	PropertyMetadata(box_value(L""), _OnTitleChanged)
);

DependencyProperty SettingsCard::DescriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::SettingsCard>(),
	PropertyMetadata(nullptr, &SettingsCard::_OnDescriptionChanged)
);

DependencyProperty SettingsCard::IconProperty = DependencyProperty::Register(
	L"Icon",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::SettingsCard>(),
	PropertyMetadata(nullptr, &SettingsCard::_OnIconChanged)
);

DependencyProperty SettingsCard::ActionContentProperty = DependencyProperty::Register(
	L"ActionContent",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::SettingsCard>(),
	PropertyMetadata(nullptr, &SettingsCard::_OnActionContentChanged)
);

void SettingsCard::_OnRawTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	SettingsCard* that = get_self<SettingsCard>(sender.as<Magpie::App::SettingsCard>());
	that->_Update();
	that->_propertyChangedEvent(*that, PropertyChangedEventArgs{ L"RawTitle" });
}

void SettingsCard::_OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	SettingsCard* that = get_self<SettingsCard>(sender.as<Magpie::App::SettingsCard>());
	that->_Update();
	that->_propertyChangedEvent(*that, PropertyChangedEventArgs{ L"Title" });
}

void SettingsCard::_OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	SettingsCard* that = get_self<SettingsCard>(sender.as<Magpie::App::SettingsCard>());
	that->_Update();
	that->_propertyChangedEvent(*that, PropertyChangedEventArgs{ L"Description" });
}

void SettingsCard::_OnIconChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	SettingsCard* that = get_self<SettingsCard>(sender.as<Magpie::App::SettingsCard>());
	that->_Update();
	that->_propertyChangedEvent(*that, PropertyChangedEventArgs{ L"Icon" });
}

void SettingsCard::_OnActionContentChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	SettingsCard* that = get_self<SettingsCard>(sender.as<Magpie::App::SettingsCard>());
	that->_Update();
	that->_propertyChangedEvent(*that, PropertyChangedEventArgs{ L"ActionContent" });
}

void SettingsCard::_Update() {
	RawTitlePresenter().Visibility(RawTitle() == nullptr ? Visibility::Collapsed : Visibility::Visible);
	TitleTextBlock().Visibility(Title().empty() ? Visibility::Collapsed : Visibility::Visible);
	DescriptionPresenter().Visibility(Description() == nullptr ? Visibility::Collapsed : Visibility::Visible);
	IconPresenter().Visibility(Icon() == nullptr ? Visibility::Collapsed : Visibility::Visible);
}

void SettingsCard::_SetEnabledState() {
	VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", true);
}

void SettingsCard::IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&) {
	_SetEnabledState();
}

void SettingsCard::Loading(FrameworkElement const&, IInspectable const&) {
	_SetEnabledState();
	_Update();
}

}
