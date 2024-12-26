﻿#pragma once
#include "SettingsExpander.g.h"
#include "Event.h"

namespace winrt::Magpie::implementation {

struct SettingsExpander : SettingsExpanderT<SettingsExpander> {
	SettingsExpander();

	static void RegisterDependencyProperties();
	static DependencyProperty HeaderProperty() { return _headerProperty; }
	static DependencyProperty DescriptionProperty() { return _descriptionProperty; }
	static DependencyProperty HeaderIconProperty() { return _headerIconProperty; }
	static DependencyProperty ContentProperty() { return _contentProperty; }
	static DependencyProperty ItemsHeaderProperty() { return _itemsHeaderProperty; }
	static DependencyProperty ItemsFooterProperty() { return _itemsFooterProperty; }
	static DependencyProperty IsExpandedProperty() { return _isExpandedProperty; }
	static DependencyProperty ItemsProperty() { return _itemsProperty; }
	static DependencyProperty ItemsSourceProperty() { return _itemsSourceProperty; }
	static DependencyProperty ItemTemplateProperty() { return _itemTemplateProperty; }

	IInspectable Header() const { return GetValue(_headerProperty); }
	void Header(IInspectable const& value) const { SetValue(_headerProperty, value); }

	IInspectable Description() const { return GetValue(_descriptionProperty); }
	void Description(IInspectable const& value) const { SetValue(_descriptionProperty, value); }

	IconElement HeaderIcon() const { return GetValue(_headerIconProperty).as<IconElement>(); }
	void HeaderIcon(IconElement const& value)const { SetValue(_headerIconProperty, value); }

	IInspectable Content() const { return GetValue(_contentProperty); }
	void Content(IInspectable const& value) const { SetValue(_contentProperty, value); }

	UIElement ItemsHeader() const { return GetValue(_itemsHeaderProperty).as<UIElement>(); }
	void ItemsHeader(UIElement const& value) const { SetValue(_itemsHeaderProperty, value); }

	UIElement ItemsFooter() const { return GetValue(_itemsFooterProperty).as<UIElement>(); }
	void ItemsFooter(UIElement const& value) const { SetValue(_itemsFooterProperty, value); }

	bool IsExpanded() const { return GetValue(_isExpandedProperty).as<bool>(); }
	void IsExpanded(bool value) const { SetValue(_isExpandedProperty, box_value(value)); }

	IVector<IInspectable> Items() const { return GetValue(_itemsProperty).as<IVector<IInspectable>>(); }
	void Items(IVector<IInspectable> const& value) const { SetValue(_itemsProperty, value); }

	IInspectable ItemsSource() const { return GetValue(_itemsSourceProperty); }
	void ItemsSource(IInspectable const& value) const { SetValue(_itemsSourceProperty, value); }

	IInspectable ItemTemplate() const { return GetValue(_itemTemplateProperty); }
	void ItemTemplate(IInspectable const& value) const { SetValue(_itemTemplateProperty, value); }

	void OnApplyTemplate();

	::Magpie::WinRTEvent<SignalDelegate> Expanded;
	::Magpie::WinRTEvent<SignalDelegate> Collapsed;

private:
	static DependencyProperty _headerProperty;
	static DependencyProperty _descriptionProperty;
	static DependencyProperty _headerIconProperty;
	static DependencyProperty _contentProperty;
	static DependencyProperty _itemsHeaderProperty;
	static DependencyProperty _itemsFooterProperty;
	static DependencyProperty _isExpandedProperty;
	static DependencyProperty _itemsProperty;
	static DependencyProperty _itemsSourceProperty;
	static DependencyProperty _itemTemplateProperty;

	static void _OnIsExpandedChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args);
	static void _OnItemsConnectedPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _OnItemsConnectedPropertyChanged();

	void _UpdateAnimatedIcon();

	MUXC::AnimatedIcon _expandCollapseChevron{ nullptr };
};

}

namespace winrt::Magpie::factory_implementation {

struct SettingsExpander : SettingsExpanderT<SettingsExpander, implementation::SettingsExpander> {
};

}
