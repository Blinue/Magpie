#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "SettingItem.g.h"


namespace winrt::Magpie::implementation {

struct SettingItem : SettingItemT<SettingItem> {
	SettingItem();

	void Title(const hstring& value);

	hstring Title() const;

	void Description(Windows::Foundation::IInspectable value);

	Windows::Foundation::IInspectable Description() const;

	void Icon(Windows::Foundation::IInspectable value);

	Windows::Foundation::IInspectable Icon() const;

	void ActionContent(Windows::Foundation::IInspectable value);

	Windows::Foundation::IInspectable ActionContent() const;

	void IsEnabledChanged(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);
	void Loading(Windows::UI::Xaml::FrameworkElement const&, Windows::Foundation::IInspectable const&);

	event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const& value);
	void PropertyChanged(event_token const& token);

	static Windows::UI::Xaml::DependencyProperty TitleProperty;
	static Windows::UI::Xaml::DependencyProperty DescriptionProperty;
	static Windows::UI::Xaml::DependencyProperty IconProperty;
	static Windows::UI::Xaml::DependencyProperty ActionContentProperty;

private:
	static void _OnTitleChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);
	static void _OnDescriptionChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);
	static void _OnIconChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);
	static void _OnActionContentChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);

	void _Update();

	void _SetEnabledState();

	event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> _propertyChangedEvent;
};

}

namespace winrt::Magpie::factory_implementation {

struct SettingItem : SettingItemT<SettingItem, implementation::SettingItem> {
};

}
