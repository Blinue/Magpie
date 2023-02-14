#pragma once
#include "KeyVisual.g.h"

namespace winrt::Magpie::App::implementation {

struct KeyVisual : KeyVisual_base<KeyVisual> {
	KeyVisual();

	void Key(int value) {
		SetValue(KeyProperty, box_value(value));
	}

	int Key() const {
		return GetValue(KeyProperty).as<int>();
	}

	void VisualType(Magpie::App::VisualType value) {
		SetValue(VisualTypeProperty, box_value(value));
	}

	Magpie::App::VisualType VisualType() const {
		return GetValue(VisualTypeProperty).as<Magpie::App::VisualType>();
	}

	void IsError(bool value) {
		SetValue(IsErrorProperty, box_value(value));
	}

	bool IsError() const {
		return GetValue(IsErrorProperty).as<bool>();
	}

	void OnApplyTemplate();

	static const DependencyProperty KeyProperty;
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
