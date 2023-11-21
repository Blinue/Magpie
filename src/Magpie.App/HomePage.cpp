#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif
#include "XamlUtils.h"

namespace winrt::Magpie::App::implementation {

void HomePage::TimerSlider_Loaded(IInspectable const& sender, RoutedEventArgs const&) const {
	// 修正 Slider 中 Tooltip 的主题
	XamlUtils::UpdateThemeOfTooltips(sender.as<Controls::Slider>(), ActualTheme());
}

}
