#include "pch.h"
#include "TextBlockHelper.h"
#if __has_include("TextBlockHelper.g.cpp")
#include "TextBlockHelper.g.cpp"
#endif
#include "Win32Utils.h"


using namespace winrt;
using namespace Windows::UI::Xaml::Controls;


namespace winrt::Magpie::UI::implementation {

DependencyProperty TextBlockHelper::_isAutoTooltipProperty = DependencyProperty::RegisterAttached(
    L"IsAutoTooltip",
    xaml_typename<bool>(),
    xaml_typename<Magpie::UI::TextBlockHelper>(),
    PropertyMetadata(box_value(false), _OnIsAutoTooltipChanged)
);

void TextBlockHelper::_OnIsAutoTooltipChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args) {
    TextBlock tb = sender.as<TextBlock>();

    bool newValue = args.NewValue().as<bool>();
    _SetTooltipBasedOnTrimmingState(tb, newValue);

    if (newValue) {
        tb.SizeChanged(_TextBlock_SizeChanged);
    } else {
        // 不支持取消
        assert(false);
    }
}

void TextBlockHelper::_TextBlock_SizeChanged(IInspectable const& sender, SizeChangedEventArgs const&) {
    _SetTooltipBasedOnTrimmingState(sender.as<TextBlock>(), true);
}

void TextBlockHelper::_SetTooltipBasedOnTrimmingState(const TextBlock& tb, bool isAttached) {
    bool hasTooltip = isAttached && tb.IsTextTrimmed();

    if (hasTooltip) {
        if (Win32Utils::GetOSBuild() < 22000) {
            // 显式设置 Tooltip 的主题
            ToolTip tooltip;
            tooltip.Content(box_value(tb.Text()));
            tooltip.RequestedTheme(tb.ActualTheme());
            ToolTipService::SetToolTip(tb, tooltip);
        } else {
            ToolTipService::SetToolTip(tb, box_value(tb.Text()));
        }
    } else {
        ToolTipService::SetToolTip(tb, nullptr);
    }
}

}
