#pragma once
#include "SimpleStackPanel.g.h"

namespace winrt::Magpie::implementation {

struct SimpleStackPanel : SimpleStackPanelT<SimpleStackPanel> {
	static DependencyProperty OrientationProperty() { return _orientationProperty; }
	static DependencyProperty PaddingProperty() { return _paddingProperty; }
	static DependencyProperty SpacingProperty() { return _spacingProperty; }

	Controls::Orientation Orientation() const { return GetValue(_orientationProperty).as<Controls::Orientation>(); }
	void Orientation(Controls::Orientation value) const { SetValue(_orientationProperty, box_value(value)); }

	Thickness Padding() const { return GetValue(_paddingProperty).as<Thickness>(); }
	void Padding(const Thickness& value) const { SetValue(_paddingProperty, box_value(value)); }

	double Spacing() const { return GetValue(_spacingProperty).as<double>(); }
	void Spacing(double value) const { SetValue(_spacingProperty, box_value(value)); }

	Size MeasureOverride(const Size& availableSize) const;

	Size ArrangeOverride(Size finalSize) const;

private:
	static const DependencyProperty _orientationProperty;
	static const DependencyProperty _paddingProperty;
	static const DependencyProperty _spacingProperty;

	static void _OnLayoutPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
};

}

namespace winrt::Magpie::factory_implementation {

struct SimpleStackPanel : SimpleStackPanelT<SimpleStackPanel, implementation::SimpleStackPanel> {
};

}
