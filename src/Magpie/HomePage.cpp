#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif
#include "XamlHelper.h"
#include "ControlHelper.h"

using namespace ::Magpie;

namespace winrt::Magpie::implementation {

void HomePage::TimerSlider_Loaded(IInspectable const& sender, RoutedEventArgs const&) const {
	// 修正 Slider 中 Tooltip 的主题
	XamlHelper::UpdateThemeOfTooltips(sender.as<Slider>(), ActualTheme());
}

void HomePage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) const {
	ControlHelper::ComboBox_DropDownOpened(sender);
}

}
