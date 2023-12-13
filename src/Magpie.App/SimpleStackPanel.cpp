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
	PropertyMetadata(box_value(Orientation::Horizontal), &SimpleStackPanel::_OnLayoutPropertyChanged)
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

Size SimpleStackPanel::MeasureOverride(const Size& availableSize) {
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
		if (item.Visibility() == Visibility::Collapsed) {
			// 不可见的子项不添加间距
			continue;
		}

		item.Measure(childAvailableSize);
		const Size itemSize = item.DesiredSize();

		if (firstItem) {
			finalSize.Height += itemSize.Height;
			firstItem = false;
		} else {
			finalSize.Height += spacing + itemSize.Height;
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

		finalSize.Width = std::max(finalSize.Width, itemSize.Width + paddings.Width);
	}
	
	return finalSize;
}

Size SimpleStackPanel::ArrangeOverride(Size finalSize) const {
	const Controls::Orientation orientation = Orientation();
	const Thickness padding = Padding();
	const float spacing = (float)Spacing();

	Point position{ (float)padding.Left, (float)padding.Top };

	for (UIElement const& item : Children()) {
		if (item.Visibility() == Visibility::Collapsed) {
			// 不可见的子项不添加间距
			continue;
		}

		auto alignment = HorizontalAlignment::Left;
		if (FrameworkElement elem = item.try_as<FrameworkElement>()) {
			alignment = elem.HorizontalAlignment();
		}

		const Size itemSize = item.DesiredSize();
		Rect itemRect{ position.X, position.Y, itemSize.Width, itemSize.Height };

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

		position.Y += itemSize.Height + spacing;
	}

	return finalSize;
}

void SimpleStackPanel::_OnLayoutPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	SimpleStackPanel* that = get_self<SimpleStackPanel>(sender.as<class_type>());
	that->InvalidateMeasure();
	that->InvalidateArrange();
}

}
