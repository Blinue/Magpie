#include "pch.h"
#include "IsEqualStateTrigger.h"
#if __has_include("IsEqualStateTrigger.g.cpp")
#include "IsEqualStateTrigger.g.cpp"
#endif

namespace winrt::Magpie::App::implementation {

const DependencyProperty IsEqualStateTrigger::_valueProperty = DependencyProperty::Register(
	L"Value",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::IsEqualStateTrigger>(),
	PropertyMetadata(nullptr, &IsEqualStateTrigger::_OnPropertyChanged)
);

const DependencyProperty IsEqualStateTrigger::_toProperty = DependencyProperty::Register(
	L"To",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::IsEqualStateTrigger>(),
	PropertyMetadata(nullptr, &IsEqualStateTrigger::_OnPropertyChanged)
);

void IsEqualStateTrigger::_OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<IsEqualStateTrigger>(sender.as<Magpie::App::IsEqualStateTrigger>())->_UpdateTrigger();
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

	// IsEqualStateTrigger 目前只在 SettingsCard 中用于比较枚举类型，其他类型都不支持。
	// 与 C# 不同，C++ 无法在运行时动态转换类型，或者需要付出很大的代价才能实现。
	if (IPropertyValue v1 = value1.try_as<IPropertyValue>()) {
		IPropertyValue v2 = value1.as<IPropertyValue>();

		if (!v1.IsNumericScalar()) {
			return false;
		}

		return v1.GetInt32() == v2.GetInt32();
	} else {
		return false;
	}
}

void IsEqualStateTrigger::_UpdateTrigger() {
	SetActive(AreValuesEqual(Value(), To()));
}

}
