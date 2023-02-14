#pragma once
#include "WrapPanel.g.h"
#include "SmallVector.h"

namespace winrt::Magpie::App::implementation {

// 移植自 https://github.com/CommunityToolkit/WindowsCommunityToolkit/tree/77b009ddf591b78dfc5bad0088c99ce35406170b/Microsoft.Toolkit.Uwp.UI.Controls.Primitives/WrapPanel

struct UvMeasure {
	UvMeasure() : u(0), v(0) {}

	UvMeasure(float u_, float v_) : u(u_), v(v_) {}

	UvMeasure(Controls::Orientation orientation, float width, float height) {
		if (orientation == Controls::Orientation::Horizontal) {
			u = width;
			v = height;
		} else {
			u = height;
			v = width;
		}
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

	void Add(const UvMeasure& position, const UvMeasure& size_) {
		childrenRects.emplace_back(position, size);

		size.u = position.u + size_.u;
		size.v = std::max(size.v, size_.v);
	}

	SmallVector<UvRect> childrenRects;
	UvMeasure size;
};

struct WrapPanel : WrapPanelT<WrapPanel> {
	double HorizontalSpacing() const {
		return GetValue(HorizontalSpacingProperty).as<double>();
	}

	void HorizontalSpacing(double value) {
		SetValue(HorizontalSpacingProperty, box_value(value));
	}

	double VerticalSpacing() const {
		return GetValue(VerticalSpacingProperty).as<double>();
	}

	void VerticalSpacing(double value) {
		SetValue(VerticalSpacingProperty, box_value(value));
	}

	Controls::Orientation Orientation() const {
		return GetValue(OrientationProperty).as<Controls::Orientation>();
	}

	void Orientation(Controls::Orientation value) {
		SetValue(OrientationProperty, box_value(value));
	}

	Thickness Padding() const {
		return GetValue(PaddingProperty).as<Thickness>();
	}

	void Padding(const Thickness& value) {
		SetValue(PaddingProperty, box_value(value));
	}

	StretchChild StretchChild() const {
		return GetValue(StretchChildProperty).as<Magpie::App::StretchChild>();
	}

	void StretchChild(Magpie::App::StretchChild value) {
		SetValue(StretchChildProperty, box_value(value));
	}

	static const DependencyProperty HorizontalSpacingProperty;
	static const DependencyProperty VerticalSpacingProperty;
	static const DependencyProperty OrientationProperty;
	static const DependencyProperty PaddingProperty;
	static const DependencyProperty StretchChildProperty;

private:
	static void _OnLayoutPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	SmallVector<Row, 0> _rows;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct WrapPanel : WrapPanelT<WrapPanel, implementation::WrapPanel> {
};

}
