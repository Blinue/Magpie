#pragma once

#include "pch.h"
#include "PageFrame.g.h"


namespace winrt::Magpie::UI::implementation {

struct PageFrame : PageFrameT<PageFrame> {
	PageFrame();

	void Title(const hstring& value);

	hstring Title() const;

	void Icon(Controls::IconElement const& value);

	Controls::IconElement Icon() const;

	void HeaderAction(FrameworkElement const& value);

	FrameworkElement HeaderAction() const;

	void MainContent(IInspectable const& value);

	IInspectable MainContent() const;

	void Loading(FrameworkElement const&, IInspectable const&);

	void ScrollViewer_PointerPressed(IInspectable const&, Input::PointerRoutedEventArgs const&);
	void ScrollViewer_ViewChanging(IInspectable const&, Controls::ScrollViewerViewChangingEventArgs const&);

	event_token PropertyChanged(Data::PropertyChangedEventHandler const& value);
	void PropertyChanged(event_token const& token);

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

	void _UpdateHeaderStyle();

	event<Data::PropertyChangedEventHandler> _propertyChangedEvent;

	Microsoft::UI::Xaml::Controls::NavigationView _rootNavigationView{ nullptr };
	Microsoft::UI::Xaml::Controls::NavigationView::DisplayModeChanged_revoker _displayModeChangedRevoker{};
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct PageFrame : PageFrameT<PageFrame, implementation::PageFrame> {
};

}
