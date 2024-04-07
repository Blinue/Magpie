#include "pch.h"
#include "SimpleStackPanel.h"
#if __has_include("SimpleStackPanel.g.cpp")
#include "SimpleStackPanel.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;

namespace winrt::Magpie::App::implementation {

const DependencyProperty SimpleStackPanel::_orientationProperty = DependencyProperty::Register(
	L"Orientation",
	xaml_typename<Controls::Orientation>(),
	xaml_typename<class_type>(),
	PropertyMetadata(box_value(Orientation::Vertical), &SimpleStackPanel::_OnLayoutPropertyChanged)
);

const DependencyProperty SimpleStackPanel::_paddingProperty = DependencyProperty::Register(
	L"Padding",
	xaml_typename<Thickness>(),
	xaml_typename<class_type>(),
	PropertyMetadata(box_value(Thickness{}), &SimpleStackPanel::_OnLayoutPropertyChanged)
);

const DependencyProperty SimpleStackPanel::_spacingProperty = DependencyProperty::Register(
	L"Spacing",
	xaml_typename<double>(),
	xaml_typename<class_type>(),
	PropertyMetadata(box_value(0.0), &SimpleStackPanel::_OnLayoutPropertyChanged)
);

Size SimpleStackPanel::MeasureOverride(const Size& availableSize) const {
	const bool isVertical = Orientation() == Orientation::Vertical;
	const float spacing = (float)Spacing();
	const Thickness padding = Padding();
	const Size paddings{ (float)padding.Left + (float)padding.Right,(float)padding.Top + (float)padding.Bottom };

	const Size childAvailableSize{
		availableSize.Width - paddings.Width,
		availableSize.Height - paddings.Height
	};

	bool firstItem = true;
	bool anyStretch = false;
	Size finalSize{ paddings.Width, paddings.Height };

	for (UIElement const& item : Children()) {
		// 调用 Measure 可以初始化绑定，因此即使子项不可见也要调用
		item.Measure(childAvailableSize);

		if (item.Visibility() == Visibility::Collapsed) {
			// 不可见的子项不添加间距
			continue;
		}

		const Size itemSize = item.DesiredSize();

		if (isVertical) {
			finalSize.Height += itemSize.Height;
			if (firstItem) {
				firstItem = false;
			} else {
				finalSize.Height += spacing;
			}

			if (anyStretch) {
				continue;
			}

			if (!std::isinf(availableSize.Width)) {
				FrameworkElement elem = item.try_as<FrameworkElement>();
				if (elem && elem.HorizontalAlignment() == HorizontalAlignment::Stretch) {
					anyStretch = true;
					finalSize.Width = availableSize.Width;
					continue;
				}
			}

			if (itemSize.Height > 0) {
				finalSize.Width = std::max(finalSize.Width, itemSize.Width + paddings.Width);
			}
		} else {
			finalSize.Width += itemSize.Width;
			if (firstItem) {
				firstItem = false;
			} else {
				finalSize.Width += spacing;
			}

			if (anyStretch) {
				continue;
			}

			if (!std::isinf(availableSize.Height)) {
				FrameworkElement elem = item.try_as<FrameworkElement>();
				if (elem && elem.VerticalAlignment() == VerticalAlignment::Stretch) {
					anyStretch = true;
					finalSize.Height = availableSize.Height;
					continue;
				}
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

	Point position{ (float)padding.Left, (float)padding.Top };

	for (UIElement const& item : Children()) {
		if (item.Visibility() == Visibility::Collapsed) {
			// 不可见的子项不添加间距
			continue;
		}

		const Size itemSize = item.DesiredSize();
		Rect itemRect{ position.X, position.Y, itemSize.Width, itemSize.Height };

		if (isVertical) {
			auto alignment = HorizontalAlignment::Left;
			if (FrameworkElement elem = item.try_as<FrameworkElement>()) {
				alignment = elem.HorizontalAlignment();
			}

			switch (alignment) {
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

void SimpleStackPanel::_OnLayoutPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	SimpleStackPanel* that = get_self<SimpleStackPanel>(sender.as<class_type>());
	that->InvalidateMeasure();
	that->InvalidateArrange();
}

}
