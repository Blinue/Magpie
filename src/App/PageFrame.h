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

	void MainContent(Windows::Foundation::IInspectable value);

	Windows::Foundation::IInspectable MainContent() const;

	static Windows::UI::Xaml::DependencyProperty TitleProperty;
	static Windows::UI::Xaml::DependencyProperty MainContentProperty;

private:
	static void _OnPropertyChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);
};

}

namespace winrt::Magpie::factory_implementation {

struct PageFrame : PageFrameT<PageFrame, implementation::PageFrame> {
};

}
