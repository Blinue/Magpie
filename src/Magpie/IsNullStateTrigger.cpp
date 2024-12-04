#include "pch.h"
#include "IsNullStateTrigger.h"
#if __has_include("IsNullStateTrigger.g.cpp")
#include "IsNullStateTrigger.g.cpp"
#endif

namespace winrt::Magpie::implementation {

const DependencyProperty IsNullStateTrigger::_valueProperty = DependencyProperty::Register(
	L"Value",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::IsNullStateTrigger>(),
	PropertyMetadata(nullptr, &IsNullStateTrigger::_OnValueChanged)
);

IsNullStateTrigger::IsNullStateTrigger() {
	_UpdateTrigger();
}

void IsNullStateTrigger::_OnValueChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<IsNullStateTrigger>(sender.as<Magpie::IsNullStateTrigger>())->_UpdateTrigger();
}

void IsNullStateTrigger::_UpdateTrigger() {
	SetActive(!Value());
}

}
