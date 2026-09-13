#pragma once
#include "IsEqualStateTrigger.g.h"

namespace winrt::Magpie::implementation {

struct IsEqualStateTrigger : IsEqualStateTriggerT<IsEqualStateTrigger> {
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, Value, _valueProperty)
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, To, _toProperty)

public:
	IsEqualStateTrigger();

private:
	static void _RegisterDependencyProperties();

	static void _OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _UpdateTrigger();
};

}

BASIC_FACTORY(IsEqualStateTrigger)
