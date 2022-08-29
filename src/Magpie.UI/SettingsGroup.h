#pragma once

#include "pch.h"
#include "SettingsGroup.g.h"


namespace winrt::Magpie::UI::implementation {

struct SettingsGroup : SettingsGroupT<SettingsGroup> {
	SettingsGroup();

	void Title(const hstring& value);

	hstring Title() const;

	void Description(IInspectable value);

	IInspectable Description() const;

	Controls::UIElementCollection Children() const;
	void Children(Controls::UIElementCollection const& value);

	void IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&);
	void Loading(FrameworkElement const&, IInspectable const&);

	event_token PropertyChanged(Data::PropertyChangedEventHandler const& value);
	void PropertyChanged(event_token const& token);

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

namespace winrt::Magpie::UI::factory_implementation {

struct SettingsGroup : SettingsGroupT<SettingsGroup, implementation::SettingsGroup> {
};

}
