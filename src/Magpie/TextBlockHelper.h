#pragma once
#include "TextBlockHelper.g.h"

namespace winrt::Magpie::implementation {

// 当 TextBlock 被截断时自动设置 Tooltip
// https://stackoverflow.com/questions/21615593/how-can-i-automatically-show-a-tooltip-if-the-text-is-too-long
struct TextBlockHelper {
    static void RegisterDependencyProperties();
    static DependencyProperty IsAutoTooltipProperty() { return _isAutoTooltipProperty; }

    static bool GetIsAutoTooltip(DependencyObject target) {
        return unbox_value<bool>(target.GetValue(_isAutoTooltipProperty));
    }

    static void SetIsAutoTooltip(DependencyObject target, bool value) {
        target.SetValue(_isAutoTooltipProperty, box_value(value));
    }

private:
    static DependencyProperty _isAutoTooltipProperty;

    static void _OnIsAutoTooltipChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args);

    static void _SetTooltipBasedOnTrimmingState(const TextBlock& tb, bool isAttached);
};

}

BASIC_FACTORY(TextBlockHelper)
