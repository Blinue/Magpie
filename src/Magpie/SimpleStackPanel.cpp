#include "pch.h"
#include "SimpleStackPanel.h"
#if __has_include("SimpleStackPanel.g.cpp")
#include "SimpleStackPanel.g.cpp"
#endif

namespace winrt::Magpie::implementation {

void SimpleStackPanel::Orientation(enum Orientation value) {
	if (_orientation == value) {
		return;
	}

	_orientation = value;
	RaisePropertyChanged(L"Orientation");

	_UpdateLayout();
}

void SimpleStackPanel::Padding(const Thickness& value) {
	if (_padding == value) {
		return;
	}

	_padding = value;
	RaisePropertyChanged(L"Padding");

	_UpdateLayout();
}

void SimpleStackPanel::Spacing(double value) {
	if (_spacing == value) {
		return;
	}

	_spacing = value;
	RaisePropertyChanged(L"Spacing");

	_UpdateLayout();
}

Size SimpleStackPanel::MeasureOverride(const Size& availableSize) const {
	const bool isVertical = _orientation == Orientation::Vertical;
	const float spacing = (float)_spacing;
	const Size paddings = {
		float(_padding.Left + _padding.Right),
		float(_padding.Top + _padding.Bottom)
	};

	const Size childAvailableSize = {
		availableSize.Width - paddings.Width,
		availableSize.Height - paddings.Height
	};

	bool isFirstItem = true;
	Size finalSize = { paddings.Width, paddings.Height };

	for (const UIElement& item : Children()) {
		// 调用 Measure 可以初始化绑定，因此即使子项不可见也要调用
		item.Measure(childAvailableSize);

		if (item.Visibility() == Visibility::Collapsed) {
			// 不可见的子项不添加间距
			continue;
		}

		Size itemSize = item.DesiredSize();

		if (isVertical) {
			finalSize.Height += itemSize.Height;
			if (isFirstItem) {
				isFirstItem = false;
			} else {
				finalSize.Height += spacing;
			}

			if (itemSize.Height > 0) {
				finalSize.Width = std::max(finalSize.Width, itemSize.Width + paddings.Width);
			}
		} else {
			finalSize.Width += itemSize.Width;
			if (isFirstItem) {
				isFirstItem = false;
			} else {
				finalSize.Width += spacing;
			}

			if (itemSize.Width > 0) {
				finalSize.Height = std::max(finalSize.Height, itemSize.Height + paddings.Height);
			}
		}
	}
	
	return finalSize;
}

Size SimpleStackPanel::ArrangeOverride(Size finalSize) const {
	const bool isVertical = Orientation() == Orientation::Vertical;
	const Thickness padding = Padding();
	const float spacing = (float)Spacing();

	Point position = { (float)padding.Left, (float)padding.Top };

	for (const UIElement& item : Children()) {
		if (item.Visibility() == Visibility::Collapsed) {
			// 不可见的子项不添加间距
			continue;
		}

		Size itemSize = item.DesiredSize();
		Rect itemRect = { position.X, position.Y, itemSize.Width, itemSize.Height };

		if (isVertical) {
			auto alignment = HorizontalAlignment::Left;
			if (FrameworkElement elem = item.try_as<FrameworkElement>()) {
				alignment = elem.HorizontalAlignment();
			}

			switch (alignment) {
			case HorizontalAlignment::Left:
				break;
			case HorizontalAlignment::Center:
				itemRect.X = position.X + (finalSize.Width - position.X - (float)padding.Right - itemRect.Width) / 2;
				break;
			case HorizontalAlignment::Right:
				itemRect.X = finalSize.Width - (float)padding.Right - itemRect.Width;
				break;
			case HorizontalAlignment::Stretch:
				itemRect.Width = finalSize.Width - position.X - (float)padding.Right;
				break;
			}
			item.Arrange(itemRect);

			if (itemSize.Height > 0) {
				position.Y += itemSize.Height + spacing;
			}
		} else {
			auto alignment = VerticalAlignment::Top;
			if (FrameworkElement elem = item.try_as<FrameworkElement>()) {
				alignment = elem.VerticalAlignment();
			}

			switch (alignment) {
			case VerticalAlignment::Top:
				break;
			case VerticalAlignment::Center:
				itemRect.Y = position.Y + (finalSize.Height - position.Y - (float)padding.Bottom - itemRect.Height) / 2;
				break;
			case VerticalAlignment::Bottom:
				itemRect.Y = finalSize.Height - (float)padding.Bottom - itemRect.Height;
				break;
			case VerticalAlignment::Stretch:
				itemRect.Height = finalSize.Height - position.Y - (float)padding.Bottom;
				break;
			}
			item.Arrange(itemRect);

			if (itemSize.Width > 0) {
				position.X += itemSize.Width + spacing;
			}
		}
	}

	return finalSize;
}

void SimpleStackPanel::_UpdateLayout() const {
	InvalidateMeasure();
	InvalidateArrange();
}

}
