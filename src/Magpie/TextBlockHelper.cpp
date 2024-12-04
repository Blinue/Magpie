#include "pch.h"
#include "TextBlockHelper.h"
#if __has_include("TextBlockHelper.g.cpp")
#include "TextBlockHelper.g.cpp"
#endif
#include "Win32Utils.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;

namespace winrt::Magpie::implementation {

DependencyProperty TextBlockHelper::_isAutoTooltipProperty = DependencyProperty::RegisterAttached(
    L"IsAutoTooltip",
    xaml_typename<bool>(),
    xaml_typename<class_type>(),
    PropertyMetadata(box_value(false), _OnIsAutoTooltipChanged)
);

void TextBlockHelper::_OnIsAutoTooltipChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args) {
    TextBlock tb = sender.as<TextBlock>();

    bool newValue = args.NewValue().as<bool>();
    _SetTooltipBasedOnTrimmingState(tb, newValue);

    if (newValue) {
        tb.SizeChanged([](IInspectable const& sender, SizeChangedEventArgs const&) {
            _SetTooltipBasedOnTrimmingState(sender.as<TextBlock>(), true);
        });
        tb.RegisterPropertyChangedCallback(
            tb.TextProperty(),
            [](DependencyObject const& sender, DependencyProperty const&) -> fire_and_forget {
                TextBlock tb = sender.as<TextBlock>();
                // 等待布局更新
                co_await 10ms;
                co_await tb.Dispatcher().TryRunAsync(
                    CoreDispatcherPriority::Low,
                    std::bind(&_SetTooltipBasedOnTrimmingState, tb, true)
                );
            }
        );
    } else {
        // 不支持取消
        assert(false);
    }
}

void TextBlockHelper::_SetTooltipBasedOnTrimmingState(const TextBlock& tb, bool isAttached) {
    // 检查是否存在 tooltip
    if (isAttached && tb.IsTextTrimmed()) {
        if (!Win32Utils::GetOSVersion().IsWin11()) {
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
