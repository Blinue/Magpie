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


void Setting::_OnMyHeaderChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
}

void Setting::_OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
}

void Setting::_OnIconChanged(DependencyObject const& sender,DependencyPropertyChangedEventArgs const&) {
}

}
