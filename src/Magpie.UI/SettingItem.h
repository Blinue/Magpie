#pragma once

#include "pch.h"
#include "SettingItem.g.h"


namespace winrt::Magpie::UI::implementation {

struct SettingItem : SettingItemT<SettingItem> {
	SettingItem();

	void Title(const hstring& value);

	hstring Title() const;

	void Description(IInspectable value);

	IInspectable Description() const;

	void Icon(IInspectable value);

	IInspectable Icon() const;

	void ActionContent(IInspectable value);

	IInspectable ActionContent() const;

	void IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&);
	void Loading(FrameworkElement const&, IInspectable const&);

	event_token PropertyChanged(Data::PropertyChangedEventHandler const& value);
	void PropertyChanged(event_token const& token);

	static DependencyProperty TitleProperty;
	static DependencyProperty DescriptionProperty;
	static DependencyProperty IconProperty;
	static DependencyProperty ActionContentProperty;

private:
	static void _OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnIconChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnActionContentChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _Update();

	void _SetEnabledState();

	event<Data::PropertyChangedEventHandler> _propertyChangedEvent;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct SettingItem : SettingItemT<SettingItem, implementation::SettingItem> {
};

}
