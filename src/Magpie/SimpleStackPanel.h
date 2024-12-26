#pragma once
#include "SimpleStackPanel.g.h"

namespace winrt::Magpie::implementation {

struct SimpleStackPanel : SimpleStackPanelT<SimpleStackPanel>, wil::notify_property_changed_base<SimpleStackPanel> {
	Orientation Orientation() const { return _orientation; }
	void Orientation(enum Orientation value);

	Thickness Padding() const { return _padding; }
	void Padding(const Thickness& value);

	double Spacing() const { return _spacing; }
	void Spacing(double value);

	Size MeasureOverride(const Size& availableSize) const;

	Size ArrangeOverride(Size finalSize) const;

private:
	void _UpdateLayout() const;

	enum Orientation _orientation = Orientation::Vertical;
	Thickness _padding{};
	double _spacing = 0.0;
};

}

namespace winrt::Magpie::factory_implementation {

struct SimpleStackPanel : SimpleStackPanelT<SimpleStackPanel, implementation::SimpleStackPanel> {
};

}
