#pragma once
#include "TextBlockHelper.g.h"

namespace winrt::Magpie::implementation {

// 当 TextBlock 被截断时自动设置 Tooltip
// https://stackoverflow.com/questions/21615593/how-can-i-automatically-show-a-tooltip-if-the-text-is-too-long
struct TextBlockHelper {
    static void RegisterDependencyProperties();
    static DependencyProperty IsAutoTooltipEnabledProperty() { return _isAutoTooltipEnabledProperty; }

    static bool GetIsAutoTooltipEnabled(DependencyObject target) {
        return unbox_value<bool>(target.GetValue(_isAutoTooltipEnabledProperty));
    }

    static void SetIsAutoTooltipEnabled(DependencyObject target, bool value) {
        target.SetValue(_isAutoTooltipEnabledProperty, box_value(value));
    }

private:
    static DependencyProperty _isAutoTooltipEnabledProperty;

    static void _OnIsAutoTooltipEnabledChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args);

    static void _SetTooltipBasedOnTrimmingState(const TextBlock& tb, bool isAttached);
};

}

BASIC_FACTORY(TextBlockHelper)
