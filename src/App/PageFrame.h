#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "PageFrame.g.h"


namespace winrt::Magpie::implementation {

struct PageFrame : PageFrameT<PageFrame> {
	PageFrame();

	void Title(const hstring& value);

	hstring Title() const;

	void MainContent(Windows::Foundation::IInspectable const& value);

	Windows::Foundation::IInspectable MainContent() const;

	void Loading(Windows::UI::Xaml::FrameworkElement const&, Windows::Foundation::IInspectable const&);

	void ScrollViewer_PointerPressed(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::Input::PointerRoutedEventArgs const&);
	void ScrollViewer_ViewChanging(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::Controls::ScrollViewerViewChangingEventArgs const&);

	event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const& value);
	void PropertyChanged(event_token const& token);

	static const Windows::UI::Xaml::DependencyProperty TitleProperty;
	static const Windows::UI::Xaml::DependencyProperty MainContentProperty;

private:
	static void _OnTitleChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);
	static void _OnMainContentChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);

	void _Update();

	void _UpdateHeaderStyle();

	event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> _propertyChangedEvent;

	Microsoft::UI::Xaml::Controls::NavigationView _rootNavigationView{ nullptr };
	Microsoft::UI::Xaml::Controls::NavigationView::DisplayModeChanged_revoker _displayModeChangedRevoker{};
};

}

namespace winrt::Magpie::factory_implementation {

struct PageFrame : PageFrameT<PageFrame, implementation::PageFrame> {
};

}
