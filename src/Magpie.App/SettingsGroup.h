#pragma once
#include "SettingsGroup.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsGroup : SettingsGroupT<SettingsGroup> {
	void Header(IInspectable const& value) const { SetValue(_headerProperty, value); }
	IInspectable Header() const { return GetValue(_headerProperty); }

	void Description(IInspectable value) const { SetValue(_descriptionProperty, value); }
	IInspectable Description() const { return GetValue(_descriptionProperty); }

	void IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&);
	void Loading(FrameworkElement const&, IInspectable const&);

	event_token PropertyChanged(PropertyChangedEventHandler const& value) {
		return _propertyChangedEvent.add(value);
	}

	void PropertyChanged(event_token const& token) {
		_propertyChangedEvent.remove(token);
	}

	static hstring AsStr(IInspectable const& value) {
		return value.as<hstring>();
	}

private:
	static const DependencyProperty _childrenProperty;
	static const DependencyProperty _headerProperty;
	static const DependencyProperty _descriptionProperty;

	static void _OnHeaderChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _Update();

	void _SetEnabledState();

	event<PropertyChangedEventHandler> _propertyChangedEvent;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsGroup : SettingsGroupT<SettingsGroup, implementation::SettingsGroup> {
};

}
