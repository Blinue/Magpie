#pragma once
#include "ControlSizeTrigger.g.h"

namespace winrt::Magpie::implementation {

struct ControlSizeTrigger : ControlSizeTriggerT<ControlSizeTrigger> {
	DEFINE_DEPENDENCY_PROPERTY(bool, CanTrigger, _canTriggerProperty)
	DEFINE_DEPENDENCY_PROPERTY(double, MaxWidth, _maxWidthProperty)
	DEFINE_DEPENDENCY_PROPERTY(double, MinWidth, _minWidthProperty)
	DEFINE_DEPENDENCY_PROPERTY(double, MaxHeight, _maxHeightProperty)
	DEFINE_DEPENDENCY_PROPERTY(double, MinHeight, _minHeightProperty)
	DEFINE_DEPENDENCY_PROPERTY(FrameworkElement, TargetElement, _targetElementProperty)

public:
	ControlSizeTrigger();

private:
	static void _RegisterDependencyProperties();

    static void _OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

    static void _OnTargetElementChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& );

    void _UpdateTrigger();

    FrameworkElement::SizeChanged_revoker _targetElementSizeChangedRevoker;
};

}

BASIC_FACTORY(ControlSizeTrigger)
