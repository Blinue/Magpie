#pragma once
#include "WrapPanel.g.h"
#include "SmallVector.h"

namespace winrt::Magpie::App::implementation {

struct UvMeasure {
	UvMeasure() : u(0), v(0) {}

	UvMeasure(float u_, float v_) : u(u_), v(v_) {}

	UvMeasure(Controls::Orientation orientation, Size size) :
		UvMeasure(orientation, size.Width, size.Height) {}

	UvMeasure(Controls::Orientation orientation, float width, float height) {
		if (orientation == Controls::Orientation::Horizontal) {
			u = width;
			v = height;
		} else {
			u = height;
			v = width;
		}
	}

	void Add(const UvMeasure& measure) noexcept {
		u += measure.u;
		v += measure.v;
	}

	Size ToSize(Controls::Orientation orientation) const noexcept {
		return orientation == Controls::Orientation::Horizontal ? Size(u, v) : Size(v, u);
	}

	float u;
	float v;
};

struct UvRect {
	Rect ToRect(Controls::Orientation orientation) const noexcept {
		if (orientation == Controls::Orientation::Horizontal) {
			return { position.u, position.v, size.u, size.v };
		} else {
			return { position.v, position.u, size.v, size.u };
		}
	}

	UvMeasure position;
	UvMeasure size;
};

struct Row {
	UvRect Rect() const noexcept {
		if (childrenRects.empty()) {
			return UvRect{ {}, size };
		} else {
			return UvRect{ childrenRects[0].position, size };
		}
	}

	void Add(const UvMeasure& position, const UvMeasure& size_) noexcept {
		childrenRects.emplace_back(position, size_);

		size.u = position.u + size_.u;
		size.v = std::max(size.v, size_.v);
	}

	void Clear() noexcept {
		childrenRects.clear();
		size = {};
	}

	SmallVector<UvRect> childrenRects;
	UvMeasure size;
};

struct WrapPanel : WrapPanelT<WrapPanel> {
	static DependencyProperty HorizontalSpacingProperty() { return _horizontalSpacingProperty; }
	static DependencyProperty VerticalSpacingProperty() { return _verticalSpacingProperty; }
	static DependencyProperty OrientationProperty() { return _orientationProperty; }
	static DependencyProperty PaddingProperty() { return _paddingProperty; }
	static DependencyProperty StretchChildProperty() { return _stretchChildProperty; }

	double HorizontalSpacing() const { return GetValue(_horizontalSpacingProperty).as<double>(); }
	void HorizontalSpacing(double value) const { SetValue(_horizontalSpacingProperty, box_value(value)); }

	double VerticalSpacing() const { return GetValue(_verticalSpacingProperty).as<double>(); }
	void VerticalSpacing(double value) const { SetValue(_verticalSpacingProperty, box_value(value)); }

	Controls::Orientation Orientation() const { return GetValue(_orientationProperty).as<Controls::Orientation>(); }
	void Orientation(Controls::Orientation value) const { SetValue(_orientationProperty, box_value(value)); }

	Thickness Padding() const { return GetValue(_paddingProperty).as<Thickness>(); }
	void Padding(const Thickness& value) const { SetValue(_paddingProperty, box_value(value)); }

	StretchChild StretchChild() const { return GetValue(_stretchChildProperty).as<Magpie::App::StretchChild>(); }
	void StretchChild(Magpie::App::StretchChild value) const { SetValue(_stretchChildProperty, box_value(value)); }

	Size MeasureOverride(const Size& availableSize);

	Size ArrangeOverride(Size finalSize);

private:
	static const DependencyProperty _horizontalSpacingProperty;
	static const DependencyProperty _verticalSpacingProperty;
	static const DependencyProperty _orientationProperty;
	static const DependencyProperty _paddingProperty;
	static const DependencyProperty _stretchChildProperty;

	static void _OnLayoutPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	Size _UpdateRows(Size availableSize);

	SmallVector<Row, 0> _rows;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct WrapPanel : WrapPanelT<WrapPanel, implementation::WrapPanel> {
};

}
