#pragma once
#include "ControlSizeTrigger.g.h"

namespace winrt::Magpie::implementation {

struct ControlSizeTrigger : ControlSizeTriggerT<ControlSizeTrigger> {
    static void RegisterDependencyProperties();
    static DependencyProperty CanTriggerProperty() { return _canTriggerProperty; }
    static DependencyProperty MaxWidthProperty() { return _maxWidthProperty; }
    static DependencyProperty MinWidthProperty() { return _minWidthProperty; }
    static DependencyProperty MaxHeightProperty() { return _maxHeightProperty; }
    static DependencyProperty MinHeightProperty() { return _minHeightProperty; }
    static DependencyProperty TargetElementProperty() { return _targetElementProperty; }

    bool CanTrigger() { return GetValue(_canTriggerProperty).try_as<bool>().value(); }
    void CanTrigger(bool value) { SetValue(_canTriggerProperty, box_value(value)); }

    double MaxWidth() { return GetValue(_maxWidthProperty).try_as<double>().value(); }
    void MaxWidth(double value) { SetValue(_maxWidthProperty, box_value(value)); }

    double MinWidth() { return GetValue(_minWidthProperty).try_as<double>().value(); }
    void MinWidth(double value) { SetValue(_minWidthProperty, box_value(value)); }

    double MaxHeight() { return GetValue(_maxHeightProperty).try_as<double>().value(); }
    void MaxHeight(double value) { SetValue(_maxHeightProperty, box_value(value)); }

    double MinHeight() { return GetValue(_minHeightProperty).try_as<double>().value(); }
    void MinHeight(double value) { SetValue(_minHeightProperty, box_value(value)); }

    FrameworkElement TargetElement() { return GetValue(_targetElementProperty).try_as<FrameworkElement>(); }
    void TargetElement(FrameworkElement const& value) { SetValue(_targetElementProperty, box_value(value)); }

private:
    static DependencyProperty _canTriggerProperty;
    static DependencyProperty _maxWidthProperty;
    static DependencyProperty _minWidthProperty;
    static DependencyProperty _maxHeightProperty;
    static DependencyProperty _minHeightProperty;
    static DependencyProperty _targetElementProperty;

    static void _OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
    static void _OnTargetElementChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& );

    void _UpdateTrigger();

    FrameworkElement::SizeChanged_revoker _targetElementSizeChangedRevoker;
};

}

BASIC_FACTORY(ControlSizeTrigger)
