#pragma once
#include "WrapPanel.g.h"
#include "SmallVector.h"

namespace winrt::Magpie::implementation {

struct UvMeasure {
	UvMeasure() : u(0), v(0) {}

	UvMeasure(float u_, float v_) : u(u_), v(v_) {}

	UvMeasure(Orientation orientation, Size size) :
		UvMeasure(orientation, size.Width, size.Height) {}

	UvMeasure(Orientation orientation, float width, float height) {
		if (orientation == Orientation::Horizontal) {
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

	Size ToSize(Orientation orientation) const noexcept {
		return orientation == Orientation::Horizontal ? Size(u, v) : Size(v, u);
	}

	float u;
	float v;
};

struct UvRect {
	Rect ToRect(Orientation orientation) const noexcept {
		if (orientation == Orientation::Horizontal) {
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

	::Magpie::SmallVector<UvRect> childrenRects;
	UvMeasure size;
};

struct WrapPanel : WrapPanelT<WrapPanel>, wil::notify_property_changed_base<WrapPanel> {
	double HorizontalSpacing() const { return _horizontalSpacing; }
	void HorizontalSpacing(double value);

	double VerticalSpacing() const { return _verticalSpacing; }
	void VerticalSpacing(double value);

	Orientation Orientation() const { return _orientation; }
	void Orientation(enum Orientation value);

	Thickness Padding() const { return _padding; }
	void Padding(const Thickness& value);

	StretchChild StretchChild() const { return _stretchChild; }
	void StretchChild(winrt::Magpie::StretchChild value);

	Size MeasureOverride(const Size& availableSize);

	Size ArrangeOverride(Size finalSize);

private:
	void _UpdateLayout() const;

	Size _UpdateRows(Size availableSize);

	double _horizontalSpacing = 0.0;
	double _verticalSpacing = 0.0;
	enum Orientation _orientation = Orientation::Horizontal;
	Thickness _padding{};
	winrt::Magpie::StretchChild _stretchChild = StretchChild::None;

	::Magpie::SmallVector<Row, 0> _rows;
};

}

namespace winrt::Magpie::factory_implementation {

struct WrapPanel : WrapPanelT<WrapPanel, implementation::WrapPanel> {
};

}
