#pragma once
#include "PageFrame.g.h"

namespace winrt::Magpie::App::implementation {

struct PageFrame : PageFrameT<PageFrame>, wil::notify_property_changed_base<PageFrame> {
	void Title(const hstring& value) {
		SetValue(TitleProperty, box_value(value));
	}

	hstring Title() const {
		return GetValue(TitleProperty).as<hstring>();
	}

	void Icon(Controls::IconElement const& value) {
		SetValue(IconProperty, value);
	}

	Controls::IconElement Icon() const {
		return GetValue(IconProperty).as<Controls::IconElement>();
	}

	void HeaderAction(FrameworkElement const& value) {
		SetValue(HeaderActionProperty, value);
	}

	FrameworkElement HeaderAction() const {
		return GetValue(HeaderActionProperty).as<FrameworkElement>();
	}

	void MainContent(IInspectable const& value) {
		SetValue(MainContentProperty, value);
	}

	IInspectable MainContent() const {
		return GetValue(MainContentProperty).as<IInspectable>();
	}

	void Loading(FrameworkElement const&, IInspectable const&);

	void Loaded(IInspectable const&, RoutedEventArgs const&);

	void SizeChanged(IInspectable const&, SizeChangedEventArgs const& e);

	void ScrollViewer_PointerPressed(IInspectable const&, Input::PointerRoutedEventArgs const&);
	void ScrollViewer_ViewChanging(IInspectable const&, Controls::ScrollViewerViewChangingEventArgs const&);
	void ScrollViewer_KeyDown(IInspectable const& sender, Input::KeyRoutedEventArgs const& args);

	static const DependencyProperty TitleProperty;
	static const DependencyProperty IconProperty;
	static const DependencyProperty HeaderActionProperty;
	static const DependencyProperty MainContentProperty;

private:
	static void _OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnIconChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnHeaderActionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnMainContentChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _Update();
};

}

namespace winrt::Magpie::App::factory_implementation {

struct PageFrame : PageFrameT<PageFrame, implementation::PageFrame> {
};

}
