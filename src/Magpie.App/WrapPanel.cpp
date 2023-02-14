#include "pch.h"
#include "WrapPanel.h"
#if __has_include("WrapPanel.g.cpp")
#include "WrapPanel.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;

namespace winrt::Magpie::App::implementation {

const DependencyProperty WrapPanel::HorizontalSpacingProperty = DependencyProperty::Register(
	L"HorizontalSpacing",
	xaml_typename<double>(),
	xaml_typename<Magpie::App::WrapPanel>(),
	PropertyMetadata(box_value(0.0), &WrapPanel::_OnLayoutPropertyChanged)
);

const DependencyProperty WrapPanel::VerticalSpacingProperty = DependencyProperty::Register(
	L"VerticalSpacing",
	xaml_typename<double>(),
	xaml_typename<Magpie::App::WrapPanel>(),
	PropertyMetadata(box_value(0.0), &WrapPanel::_OnLayoutPropertyChanged)
);

const DependencyProperty WrapPanel::OrientationProperty = DependencyProperty::Register(
	L"Orientation",
	xaml_typename<Controls::Orientation>(),
	xaml_typename<Magpie::App::WrapPanel>(),
	PropertyMetadata(box_value(Orientation::Horizontal), &WrapPanel::_OnLayoutPropertyChanged)
);

const DependencyProperty WrapPanel::PaddingProperty = DependencyProperty::Register(
	L"Padding",
	xaml_typename<Thickness>(),
	xaml_typename<Magpie::App::WrapPanel>(),
	PropertyMetadata(box_value(Thickness{}), &WrapPanel::_OnLayoutPropertyChanged)
);

const DependencyProperty WrapPanel::StretchChildProperty = DependencyProperty::Register(
	L"StretchChild",
	xaml_typename<Magpie::App::StretchChild>(),
	xaml_typename<Magpie::App::WrapPanel>(),
	PropertyMetadata(box_value(StretchChild::None), &WrapPanel::_OnLayoutPropertyChanged)
);

void WrapPanel::_OnLayoutPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	WrapPanel* that = get_self<WrapPanel>(sender.as<default_interface<WrapPanel>>());
	that->InvalidateMeasure();
	that->InvalidateArrange();
}

}
