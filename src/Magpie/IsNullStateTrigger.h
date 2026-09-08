#pragma once
#include "IsNullStateTrigger.g.h"

namespace winrt::Magpie::implementation {

struct IsNullStateTrigger : IsNullStateTriggerT<IsNullStateTrigger> {
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, Value, _valueProperty)

public:
	IsNullStateTrigger();

private:
	static void _RegisterDependencyProperties();

	static void _OnValueChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _UpdateTrigger();
};

}

BASIC_FACTORY(IsNullStateTrigger)
