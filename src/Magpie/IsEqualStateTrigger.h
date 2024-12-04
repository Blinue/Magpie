#pragma once
#include "IsEqualStateTrigger.g.h"

namespace winrt::Magpie::implementation {

struct IsEqualStateTrigger : IsEqualStateTriggerT<IsEqualStateTrigger> {
	IInspectable Value() const {
		return GetValue(_valueProperty);
	}

	void Value(IInspectable const& value) {
		SetValue(_valueProperty, value);
	}

	IInspectable To() const {
		return GetValue(_toProperty);
	}

	void To(IInspectable const& value) {
		SetValue(_toProperty, value);
	}

	static DependencyProperty ValueProperty() {
		return _valueProperty;
	}

	static DependencyProperty ToProperty() {
		return _toProperty;
	}

private:
	static const DependencyProperty _valueProperty;
	static const DependencyProperty _toProperty;

	static void _OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _UpdateTrigger();
};

}

namespace winrt::Magpie::factory_implementation {

struct IsEqualStateTrigger : IsEqualStateTriggerT<IsEqualStateTrigger, implementation::IsEqualStateTrigger> {
};

}
