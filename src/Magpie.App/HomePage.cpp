#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif
#include "XamlUtils.h"
#include "ComboBoxHelper.h"

namespace winrt::Magpie::App::implementation {

void HomePage::TimerSlider_Loaded(IInspectable const& sender, RoutedEventArgs const&) const {
	// 修正 Slider 中 Tooltip 的主题
	XamlUtils::UpdateThemeOfTooltips(sender.as<Controls::Slider>(), ActualTheme());
}

void HomePage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) const {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

void HomePage::SimulateExclusiveFullscreenToggleSwitch_Toggled(IInspectable const& sender, RoutedEventArgs const&) {
	// 如果没有启用开发者模式，模拟独占全屏选项位于页面底部，用户可能注意不到警告。
	// 为了解决这个问题，打开模拟独占全屏选项时自动滚动页面。
	if (_viewModel.IsDeveloperMode() || !sender.as<Controls::ToggleSwitch>().IsOn() || !IsLoaded()) {
		return;
	}

	// 这个回调被触发时 UI 还没有更新，需要异步处理
	Dispatcher().TryRunAsync(CoreDispatcherPriority::Low, [weakThis(get_weak())]() {
		auto strongThis = weakThis.get();
		if (!strongThis) {
			return;
		}

		auto scrollViewer = strongThis->PageFrame().ScrollViewer();
		scrollViewer.UpdateLayout();
		// 滚动到底部
		scrollViewer.ChangeView(nullptr, scrollViewer.ScrollableHeight(), nullptr);
	});
}

}
