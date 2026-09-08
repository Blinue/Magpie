#pragma once
#include "TextBlockHelper.g.h"

namespace winrt::Magpie::implementation {

// 当 TextBlock 被截断时自动设置 Tooltip
// https://stackoverflow.com/questions/21615593/how-can-i-automatically-show-a-tooltip-if-the-text-is-too-long
struct TextBlockHelper {
    static DependencyProperty IsAutoTooltipEnabledProperty() {
		_RegisterDependencyProperties();
		return _isAutoTooltipEnabledProperty;
	}

    static bool GetIsAutoTooltipEnabled(DependencyObject target) {
		_RegisterDependencyProperties();
        return unbox_value<bool>(target.GetValue(_isAutoTooltipEnabledProperty));
    }

    static void SetIsAutoTooltipEnabled(DependencyObject target, bool value) {
		_RegisterDependencyProperties();
        target.SetValue(_isAutoTooltipEnabledProperty, box_value(value));
    }

private:
	static void _RegisterDependencyProperties();

    static void _OnIsAutoTooltipEnabledChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args);

    static void _SetTooltipBasedOnTrimmingState(const TextBlock& tb, bool isAttached);

	static DependencyProperty _isAutoTooltipEnabledProperty;
};

}

BASIC_FACTORY(TextBlockHelper)
