#pragma once
#include "KeyVisual.g.h"

namespace winrt::Magpie::App::implementation {

struct KeyVisual : KeyVisual_base<KeyVisual> {
	KeyVisual();

	void Content(IInspectable const& value);

	IInspectable Content() const;

	void VisualType(Magpie::App::VisualType value);

	Magpie::App::VisualType VisualType() const;

	void IsError(bool value);

	bool IsError() const;

	void OnApplyTemplate();

	static const DependencyProperty ContentProperty;
	static const DependencyProperty VisualTypeProperty;
	static const DependencyProperty IsErrorProperty;

private:
	static void _OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnIsErrorChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&);

	void _Update();
	Windows::UI::Xaml::Style _GetStyleSize(std::wstring_view styleName) const;
	double _GetIconSize() const;

	void _SetErrorState();
	void _SetEnabledState();

	Controls::ContentPresenter _keyPresenter{ nullptr };
	event_token _isEnabledChangedToken{};
};

}

namespace winrt::Magpie::App::factory_implementation {

struct KeyVisual : KeyVisualT<KeyVisual, implementation::KeyVisual> {
};

}
