#pragma once
#include "IsEqualStateTrigger.g.h"

namespace winrt::Magpie::implementation {

struct IsEqualStateTrigger : IsEqualStateTriggerT<IsEqualStateTrigger> {
	static void RegisterDependencyProperties();
	static DependencyProperty ValueProperty() { return _valueProperty; }
	static DependencyProperty ToProperty() { return _toProperty; }

	IInspectable Value() const { return GetValue(_valueProperty); }
	void Value(IInspectable const& value) { SetValue(_valueProperty, value); }

	IInspectable To() const { return GetValue(_toProperty); }
	void To(IInspectable const& value) { SetValue(_toProperty, value); }

private:
	static DependencyProperty _valueProperty;
	static DependencyProperty _toProperty;

	static void _OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _UpdateTrigger();
};

}

BASIC_FACTORY(IsEqualStateTrigger)
