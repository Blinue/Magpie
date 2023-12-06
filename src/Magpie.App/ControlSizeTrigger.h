#pragma once
#include "ControlSizeTrigger.g.h"

namespace winrt::Magpie::App::implementation {

struct ControlSizeTrigger : ControlSizeTriggerT<ControlSizeTrigger> {
    bool CanTrigger() {
        return GetValue(_canTriggerProperty).as<bool>();
    }

    void CanTrigger(bool value) {
        SetValue(_canTriggerProperty, box_value(value));
    }

    double MaxWidth() {
        return GetValue(_maxWidthProperty).as<double>();
    }

    void MaxWidth(double value) {
        SetValue(_maxWidthProperty, box_value(value));
    }

    double MinWidth() {
        return GetValue(_minWidthProperty).as<double>();
    }

    void MinWidth(double value) {
        SetValue(_minWidthProperty, box_value(value));
    }

    double MaxHeight() {
        return GetValue(_maxHeightProperty).as<double>();
    }

    void MaxHeight(double value) {
        SetValue(_maxHeightProperty, box_value(value));
    }

    double MinHeight() {
        return GetValue(_minHeightProperty).as<double>();
    }

    void MinHeight(double value) {
        SetValue(_minHeightProperty, box_value(value));
    }

    FrameworkElement TargetElement() {
        return GetValue(_targetElementProperty).as<FrameworkElement>();
    }

    void TargetElement(FrameworkElement const& value) {
        SetValue(_targetElementProperty, box_value(value));
    }

    static DependencyProperty CanTriggerProperty() {
        return _canTriggerProperty;
    }

    static DependencyProperty MaxWidthProperty() {
        return _maxWidthProperty;
    }

    static DependencyProperty MinWidthProperty() {
        return _minWidthProperty;
    }

    static DependencyProperty MaxHeightProperty() {
        return _maxHeightProperty;
    }

    static DependencyProperty MinHeightProperty() {
        return _minHeightProperty;
    }

    static DependencyProperty TargetElementProperty() {
        return _targetElementProperty;
    }

private:
    static const DependencyProperty _canTriggerProperty;
    static const DependencyProperty _maxWidthProperty;
    static const DependencyProperty _minWidthProperty;
    static const DependencyProperty _maxHeightProperty;
    static const DependencyProperty _minHeightProperty;
    static const DependencyProperty _targetElementProperty;

    static void _OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
    static void _OnTargetElementChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& );

    void _UpdateTrigger();

    FrameworkElement::SizeChanged_revoker _targetElementSizeChangedRevoker;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ControlSizeTrigger : ControlSizeTriggerT<ControlSizeTrigger, implementation::ControlSizeTrigger> {
};

}
