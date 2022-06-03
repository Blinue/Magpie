#pragma once

#include "KeyVisual.g.h"


namespace winrt::Magpie::implementation {

struct KeyVisual : KeyVisual_base<KeyVisual> {
	KeyVisual();

	void Content(Windows::Foundation::IInspectable const& value);

	Windows::Foundation::IInspectable Content() const;

	void VisualType(Magpie::VisualType value);

	Magpie::VisualType VisualType() const;

	static const Windows::UI::Xaml::DependencyProperty ContentProperty;
	static const Windows::UI::Xaml::DependencyProperty VisualTypeProperty;

private:
	static void _OnPropertyChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);

	void _Update();
	Windows::UI::Xaml::Style _GetStyleSize(std::wstring_view styleName);
};

}

namespace winrt::Magpie::factory_implementation {

struct KeyVisual : KeyVisualT<KeyVisual, implementation::KeyVisual> {
};

}
