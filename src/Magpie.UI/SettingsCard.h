#pragma once

#include "pch.h"
#include "SettingsCard.g.h"


namespace winrt::Magpie::UI::implementation {

struct SettingsCard : SettingsCardT<SettingsCard> {
	SettingsCard();

	void RawTitle(IInspectable const& value);

	IInspectable RawTitle() const;

	void Title(const hstring& value);

	hstring Title() const;

	void Description(IInspectable const& value);

	IInspectable Description() const;

	void Icon(IInspectable const& value);

	IInspectable Icon() const;

	void ActionContent(IInspectable const& value);

	IInspectable ActionContent() const;

	void IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&);
	void Loading(FrameworkElement const&, IInspectable const&);

	event_token PropertyChanged(Data::PropertyChangedEventHandler const& value);
	void PropertyChanged(event_token const& token);

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

	event<Data::PropertyChangedEventHandler> _propertyChangedEvent;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct SettingsCard : SettingsCardT<SettingsCard, implementation::SettingsCard> {
};

}
