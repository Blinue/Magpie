#include "pch.h"
#include "IsEqualStateTrigger.h"
#if __has_include("IsEqualStateTrigger.g.cpp")
#include "IsEqualStateTrigger.g.cpp"
#endif

namespace winrt::Magpie::implementation {

DependencyProperty IsEqualStateTrigger::_valueProperty{ nullptr };
DependencyProperty IsEqualStateTrigger::_toProperty{ nullptr };

void IsEqualStateTrigger::RegisterDependencyProperties() {
	_valueProperty = DependencyProperty::Register(
		L"Value",
		xaml_typename<IInspectable>(),
		xaml_typename<Magpie::IsEqualStateTrigger>(),
		PropertyMetadata(nullptr, &IsEqualStateTrigger::_OnPropertyChanged)
	);

	_toProperty = DependencyProperty::Register(
		L"To",
		xaml_typename<IInspectable>(),
		xaml_typename<Magpie::IsEqualStateTrigger>(),
		PropertyMetadata(nullptr, &IsEqualStateTrigger::_OnPropertyChanged)
	);
}

void IsEqualStateTrigger::_OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<IsEqualStateTrigger>(sender.as<Magpie::IsEqualStateTrigger>())->_UpdateTrigger();
}

static bool AreValuesEqual(IInspectable const& value1, IInspectable const& value2) {
	if (value1 == value2) {
		return true;
	}

	if (!value1 || !value2) {
		return false;
	}

	if (get_class_name(value1) != get_class_name(value2)) {
		return false;
	}
	
	if (IPropertyValue v1 = value1.try_as<IPropertyValue>()) {
		IPropertyValue v2 = value1.as<IPropertyValue>();

		// 没有必要为每种类型都添加处理逻辑，IsEqualStateTrigger 目前只在 SettingsCard 中用于比较枚举类型
		switch (v1.Type()) {
		case PropertyType::OtherType:
		{
			return value1.as<IReference<ContentAlignment>>() == value2.as<IReference<ContentAlignment>>();
		}
		default:
			return false;
		}
	} else {
		return false;
	}
}

void IsEqualStateTrigger::_UpdateTrigger() {
	SetActive(AreValuesEqual(Value(), To()));
}

}
