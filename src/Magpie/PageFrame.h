#pragma once
#include "PageFrame.g.h"

namespace winrt::Magpie::implementation {

struct PageFrame : PageFrameT<PageFrame>, wil::notify_property_changed_base<PageFrame> {
	void InitializeComponent();

	hstring Title() const noexcept { return _title; }
	void Title(hstring value);

	IconElement Icon() const { return _icon; }
	void Icon(IconElement value);

	FrameworkElement HeaderAction() const { return _headerAction; }
	void HeaderAction(FrameworkElement value);

	IInspectable MainContent() const { return _mainContent; }
	void MainContent(IInspectable value);

	void Loaded(IInspectable const&, RoutedEventArgs const&);

	void SizeChanged(IInspectable const&, SizeChangedEventArgs const& e);

	void ScrollViewer_PointerPressed(IInspectable const&, Input::PointerRoutedEventArgs const&);
	void ScrollViewer_ViewChanging(IInspectable const&, ScrollViewerViewChangingEventArgs const&);
	void ScrollViewer_KeyDown(IInspectable const& sender, Input::KeyRoutedEventArgs const& args);

private:
	void _UpdateIconContainer();
	void _UpdateHeaderActionPresenter();

	hstring _title;
	IconElement _icon{ nullptr };
	FrameworkElement _headerAction{ nullptr };
	IInspectable _mainContent{ nullptr };
};

}

namespace winrt::Magpie::factory_implementation {

struct PageFrame : PageFrameT<PageFrame, implementation::PageFrame> {
};

}
