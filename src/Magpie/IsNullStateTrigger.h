#pragma once
#include "IsNullStateTrigger.g.h"

namespace winrt::Magpie::implementation {

struct IsNullStateTrigger : IsNullStateTriggerT<IsNullStateTrigger> {
	IsNullStateTrigger();

	static void RegisterDependencyProperties();
	static DependencyProperty ValueProperty() { return _valueProperty; }

	IInspectable Value() const { return GetValue(_valueProperty); }
	void Value(IInspectable const& value) { SetValue(_valueProperty, value); }

private:
	static DependencyProperty _valueProperty;

	static void _OnValueChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _UpdateTrigger();
};

}

namespace winrt::Magpie::factory_implementation {

struct IsNullStateTrigger : IsNullStateTriggerT<IsNullStateTrigger, implementation::IsNullStateTrigger> {
};

}
