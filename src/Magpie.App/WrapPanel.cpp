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

Size WrapPanel::MeasureOverride(const Size& availableSize) {
	Thickness padding = Padding();
	Size childAvailableSize{
		availableSize.Width - (float)padding.Left - (float)padding.Right,
		availableSize.Height - (float)padding.Top - (float)padding.Bottom
	};
	for (const UIElement& child : Children()) {
		child.Measure(childAvailableSize);
	}

	return _UpdateRows(availableSize);
}

Size WrapPanel::ArrangeOverride(Size finalSize) {
	Controls::Orientation orientation = Orientation();
	Size desiredSize = DesiredSize();

	if ((orientation == Orientation::Horizontal && finalSize.Width < desiredSize.Width) ||
		(orientation == Orientation::Vertical && finalSize.Height < desiredSize.Height)) {
		// We haven't received our desired size. We need to refresh the rows.
		_UpdateRows(finalSize);
	}

	if (_rows.empty()) {
		return finalSize;
	}

	UIElementCollection children = Children();

	// Now that we have all the data, we do the actual arrange pass
	uint32_t childIndex = 0;
	for (const Row& row : _rows) {
		for (const UvRect& rect : row.childrenRects) {
			UIElement child = children.GetAt(childIndex++);
			while (child.Visibility() == Visibility::Collapsed) {
				// Collapsed children are not added into the rows,
				// we skip them.
				child = children.GetAt(childIndex++);
			}

			UvRect arrangeRect{
				rect.position,
				UvMeasure(rect.size.u, row.size.v),
			};

			Rect finalRect = arrangeRect.ToRect(orientation);
			child.Arrange(finalRect);
		}
	}

	return finalSize;
}

void WrapPanel::_OnLayoutPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	WrapPanel* that = get_self<WrapPanel>(sender.as<Magpie::App::WrapPanel>());
	that->InvalidateMeasure();
	that->InvalidateArrange();
}

Size WrapPanel::_UpdateRows(Size availableSize) {
	_rows.clear();

	Controls::Orientation orientation = Orientation();
	Thickness padding = Padding();
	UIElementCollection children = Children();

	UvMeasure paddingStart(orientation, (float)padding.Left, (float)padding.Top);
	UvMeasure paddingEnd(orientation, (float)padding.Right, (float)padding.Bottom);

	if (children.Size() == 0) {
		paddingStart.Add(paddingEnd);
		return paddingStart.ToSize(orientation);
	}

	UvMeasure parentMeasure(orientation, availableSize.Width, availableSize.Height);
	UvMeasure spacingMeasure(orientation, (float)HorizontalSpacing(), (float)VerticalSpacing());
	UvMeasure position(orientation, (float)padding.Left, (float)padding.Top);

	Row currentRow;
	UvMeasure finalMeasure(orientation, 0.0f, 0.0f);

	auto arrange = [&](UIElement const& child, bool isLast = false) {
		if (child.Visibility() == Visibility::Collapsed) {
			return; // if an item is collapsed, avoid adding the spacing
		}

		UvMeasure desiredMeasure(orientation, child.DesiredSize());
		if ((desiredMeasure.u + position.u + paddingEnd.u) > parentMeasure.u) {
			// next row!
			position.u = paddingStart.u;
			position.v += currentRow.size.v + spacingMeasure.v;

			_rows.push_back(std::move(currentRow));
			currentRow = {};
		}

		// Stretch the last item to fill the available space
		if (isLast) {
			desiredMeasure.u = parentMeasure.u - position.u;
		}

		currentRow.Add(position, desiredMeasure);

		// adjust the location for the next items
		position.u += desiredMeasure.u + spacingMeasure.u;
		finalMeasure.u = std::max(finalMeasure.u, position.u);
	};

	uint32_t lastIndex = children.Size() - 1;
	for (uint32_t i = 0; i < lastIndex; i++) {
		arrange(children.GetAt(i));
	}

	arrange(children.GetAt(lastIndex), StretchChild() == StretchChild::Last);
	if (!currentRow.childrenRects.empty()) {
		_rows.push_back(std::move(currentRow));
	}

	if (_rows.empty()) {
		paddingStart.Add(paddingEnd);
		return paddingStart.ToSize(orientation);
	}

	// Get max V here before computing final rect
	UvRect lastRowRect = _rows.back().Rect();
	finalMeasure.v = lastRowRect.position.v + lastRowRect.size.v;
	finalMeasure.Add(paddingEnd);
	return finalMeasure.ToSize(orientation);
}

}
