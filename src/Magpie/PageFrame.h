#pragma once
#include "PageFrame.g.h"

namespace winrt::Magpie::implementation {

struct PageFrame : PageFrameT<PageFrame> {
	DEFINE_DEPENDENCY_PROPERTY(hstring, Title, _titleProperty)
	DEFINE_DEPENDENCY_PROPERTY(IconElement, Icon, _iconProperty)
	DEFINE_DEPENDENCY_PROPERTY(FrameworkElement, HeaderAction, _headerActionProperty)
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, MainContent, _mainContentProperty)

public:
	void InitializeComponent();

	void Loaded(IInspectable const&, RoutedEventArgs const&);

	void SizeChanged(IInspectable const&, SizeChangedEventArgs const& e);

	void ScrollViewer_PointerPressed(IInspectable const&, Input::PointerRoutedEventArgs const&);
	void ScrollViewer_ViewChanging(IInspectable const&, ScrollViewerViewChangingEventArgs const&);
	void ScrollViewer_KeyDown(IInspectable const& sender, Input::KeyRoutedEventArgs const& args);

private:
	static void _RegisterDependencyProperties();

	void _UpdateIconContainer();
	void _UpdateHeaderActionPresenter();
};

}

BASIC_FACTORY(PageFrame)
