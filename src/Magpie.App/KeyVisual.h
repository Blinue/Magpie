#pragma once
#include "KeyVisual.g.h"

namespace winrt::Magpie::App::implementation {

struct KeyVisual : KeyVisual_base<KeyVisual> {
	KeyVisual();

	void Key(int value) {
		SetValue(_keyProperty, box_value(value));
	}

	int Key() const {
		return GetValue(_keyProperty).as<int>();
	}

	void VisualType(Magpie::App::VisualType value) {
		SetValue(_visualTypeProperty, box_value(value));
	}

	Magpie::App::VisualType VisualType() const {
		return GetValue(_visualTypeProperty).as<Magpie::App::VisualType>();
	}

	void IsError(bool value) {
		SetValue(_isErrorProperty, box_value(value));
	}

	bool IsError() const {
		return GetValue(_isErrorProperty).as<bool>();
	}

	void OnApplyTemplate();

	static DependencyProperty KeyProperty() {
		return _keyProperty;
	}

	static DependencyProperty VisualTypeProperty() {
		return _visualTypeProperty;
	}

	static DependencyProperty IsErrorProperty() {
		return _isErrorProperty;
	}

private:
	static const DependencyProperty _keyProperty;
	static const DependencyProperty _visualTypeProperty;
	static const DependencyProperty _isErrorProperty;

	static void _OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnIsErrorChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&);

	void _Update();
	Windows::UI::Xaml::Style _GetStyleSize(std::wstring_view styleName) const;
	double _GetIconSize() const;

	void _SetErrorState();
	void _SetEnabledState();

	Controls::ContentPresenter _keyPresenter{ nullptr };
	IsEnabledChanged_revoker _isEnabledChangedRevoker;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct KeyVisual : KeyVisualT<KeyVisual, implementation::KeyVisual> {
};

}
