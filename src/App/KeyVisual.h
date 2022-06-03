#pragma once

#include "KeyVisual.g.h"


namespace winrt::Magpie::implementation {

struct KeyVisual : KeyVisual_base<KeyVisual> {
	KeyVisual();

	void Content(Windows::Foundation::IInspectable const& value);

	Windows::Foundation::IInspectable Content() const;

	void VisualType(Magpie::VisualType value);

	Magpie::VisualType VisualType() const;

	void IsError(bool value);

	bool IsError() const;

	void OnApplyTemplate();

	static const Windows::UI::Xaml::DependencyProperty ContentProperty;
	static const Windows::UI::Xaml::DependencyProperty VisualTypeProperty;
	static const Windows::UI::Xaml::DependencyProperty IsErrorProperty;

private:
	static void _OnPropertyChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);
	static void _OnIsErrorChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);

	void _IsEnabledChanged(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);

	void _Update();
	Windows::UI::Xaml::Style _GetStyleSize(std::wstring_view styleName) const;
	double _GetIconSize() const;

	void _SetErrorState();
	void _SetEnabledState();

	Windows::UI::Xaml::Controls::ContentPresenter _keyPresenter{ nullptr };
	event_token _isEnabledChangedToken{};
};

}

namespace winrt::Magpie::factory_implementation {

struct KeyVisual : KeyVisualT<KeyVisual, implementation::KeyVisual> {
};

}
