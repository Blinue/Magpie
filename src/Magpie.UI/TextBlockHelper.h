#pragma once
#include "TextBlockHelper.g.h"

namespace winrt::Magpie::UI::implementation {

// 当 TextBlock 被截断时自动设置 Tooltip
// https://stackoverflow.com/questions/21615593/how-can-i-automatically-show-a-tooltip-if-the-text-is-too-long

struct TextBlockHelper : TextBlockHelperT<TextBlockHelper> {
    TextBlockHelper() = default;

    static DependencyProperty IsAutoTooltipProperty() noexcept {
        return _isAutoTooltipProperty;
    }

    static bool GetIsAutoTooltip(DependencyObject target) {
        return unbox_value<bool>(target.GetValue(_isAutoTooltipProperty));
    }

    static void SetIsAutoTooltip(DependencyObject target, bool value) {
        target.SetValue(_isAutoTooltipProperty, box_value(value));
    }

private:
    static DependencyProperty _isAutoTooltipProperty;

    static void _OnIsAutoTooltipChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args);

    static void _SetTooltipBasedOnTrimmingState(const Controls::TextBlock& tb, bool isAttached);
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct TextBlockHelper : TextBlockHelperT<TextBlockHelper, implementation::TextBlockHelper> {
};

}
