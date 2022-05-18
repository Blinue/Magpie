#include "pch.h"
#include "Setting.h"
#if __has_include("Controls.Setting.g.cpp")
#include "Controls.Setting.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;
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
	auto name = name_of<Magpie::App::Controls::Setting>();
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
	/*IsEnabledChanged -= Setting_IsEnabledChanged;
	_setting = (Setting)this;
	_setting.GetTemplateChild(_PartIconPresenter).as(_iconPresenter);
	_setting.GetTemplateChild(_PartDescriptionPresenter).as(_descriptionPresenter);
	_Update();
	_SetEnabledState();
	IsEnabledChanged += Setting_IsEnabledChanged;*/

	__super::OnApplyTemplate();
}

void Setting::_Update() {

}

void Setting::_Setting_IsEnabledChanged(Windows::Foundation::IInspectable const&, DependencyPropertyChangedEventArgs const&) {
	_SetEnabledState();
}

void Setting::_SetEnabledState() {
	VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", true);
}


void Setting::_OnMyHeaderChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	//winrt::get_self<Setting>(sender)->_Update();
}

void Setting::_OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
}

void Setting::_OnIconChanged(DependencyObject const& sender,DependencyPropertyChangedEventArgs const&) {
}

}
