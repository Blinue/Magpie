#pragma once
#include "SettingsCard.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsCard : SettingsCardT<SettingsCard> {
	void RawTitle(IInspectable const& value) {
		SetValue(RawTitleProperty, value);
	}

	IInspectable RawTitle() const {
		return GetValue(RawTitleProperty);
	}

	void Title(const hstring& value) {
		SetValue(TitleProperty, box_value(value));
	}

	hstring Title() const {
		return GetValue(TitleProperty).as<hstring>();
	}

	void Description(IInspectable const& value) {
		SetValue(DescriptionProperty, value);
	}

	IInspectable Description() const {
		return GetValue(DescriptionProperty);
	}

	void Icon(IInspectable const& value) {
		SetValue(IconProperty, value);
	}

	IInspectable Icon() const {
		return GetValue(IconProperty);
	}

	void ActionContent(IInspectable const& value) {
		SetValue(ActionContentProperty, value);
	}

	IInspectable ActionContent() const {
		return GetValue(ActionContentProperty);
	}

	void IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&);
	void Loading(FrameworkElement const&, IInspectable const&);

	event_token PropertyChanged(PropertyChangedEventHandler const& value) {
		return _propertyChangedEvent.add(value);
	}

	void PropertyChanged(event_token const& token) {
		_propertyChangedEvent.remove(token);
	}

	static DependencyProperty RawTitleProperty;
	static DependencyProperty TitleProperty;
	static DependencyProperty DescriptionProperty;
	static DependencyProperty IconProperty;
	static DependencyProperty ActionContentProperty;

private:
	static void _OnRawTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnIconChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnActionContentChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _Update();

	void _SetEnabledState();

	event<PropertyChangedEventHandler> _propertyChangedEvent;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsCard : SettingsCardT<SettingsCard, implementation::SettingsCard> {
};

}
