#pragma once
#include "SettingsGroup.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsGroup : SettingsGroupT<SettingsGroup> {
	SettingsGroup();

	void Title(const hstring& value) {
		SetValue(TitleProperty, box_value(value));
	}

	hstring Title() const {
		return GetValue(TitleProperty).as<hstring>();
	}

	void Description(IInspectable value) {
		SetValue(DescriptionProperty, value);
	}

	IInspectable Description() const {
		return GetValue(DescriptionProperty);
	}

	Controls::UIElementCollection Children() const {
		return GetValue(ChildrenProperty).as<Controls::UIElementCollection>();
	}

	void Children(Controls::UIElementCollection const& value) {
		SetValue(ChildrenProperty, value);
	}

	void IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&);
	void Loading(FrameworkElement const&, IInspectable const&);

	event_token PropertyChanged(Data::PropertyChangedEventHandler const& value) {
		return _propertyChangedEvent.add(value);
	}

	void PropertyChanged(event_token const& token) {
		_propertyChangedEvent.remove(token);
	}

	static const DependencyProperty ChildrenProperty;
	static const DependencyProperty TitleProperty;
	static const DependencyProperty DescriptionProperty;

private:
	static void _OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _Update();

	void _SetEnabledState();

	event<Data::PropertyChangedEventHandler> _propertyChangedEvent;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsGroup : SettingsGroupT<SettingsGroup, implementation::SettingsGroup> {
};

}
