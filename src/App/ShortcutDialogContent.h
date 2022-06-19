#pragma once

#include "pch.h"
#include "ShortcutDialogContent.g.h"


namespace winrt::Magpie::App::implementation {

struct ShortcutDialogContent : ShortcutDialogContentT<ShortcutDialogContent> {
	ShortcutDialogContent();

	void IsError(bool value);
	bool IsError() const;

	void Keys(const IVector<IInspectable>& value);
	IVector<IInspectable> Keys() const;

	event_token PropertyChanged(Data::PropertyChangedEventHandler const& value);
	void PropertyChanged(event_token const& token);

	static const DependencyProperty IsErrorProperty;
	static const DependencyProperty KeysProperty;

private:
	static void _OnIsErrorChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnKeysChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	event<Data::PropertyChangedEventHandler> _propertyChangedEvent;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ShortcutDialogContent : ShortcutDialogContentT<ShortcutDialogContent, implementation::ShortcutDialogContent> {
};

}
