#pragma once
#include "IsNullStateTrigger.g.h"

namespace winrt::Magpie::App::implementation {

struct IsNullStateTrigger : IsNullStateTriggerT<IsNullStateTrigger> {
	IsNullStateTrigger();

	IInspectable Value() const {
		return GetValue(_valueProperty);
	}

	void Value(IInspectable const& value) {
		SetValue(_valueProperty, value);
	}

	static DependencyProperty ValueProperty() {
		return _valueProperty;
	}

private:
	static const DependencyProperty _valueProperty;

	static void _OnValueChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _UpdateTrigger();
};

}

namespace winrt::Magpie::App::factory_implementation {

struct IsNullStateTrigger : IsNullStateTriggerT<IsNullStateTrigger, implementation::IsNullStateTrigger> {
};

}
