#pragma once
#include "SettingsExpanderItemStyleSelector.g.h"
#include "SettingsExpander.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsExpanderItemStyleSelector : SettingsExpanderItemStyleSelectorT<SettingsExpanderItemStyleSelector> {
	Style DefaultStyle() const {
		return _defaultStyle;
	}

	void DefaultStyle(Style const& value) {
		_defaultStyle = value;
	}

	Style ClickableStyle() const {
		return _clickableStyle;
	}

	void ClickableStyle(Style const& value) {
		_clickableStyle = value;
	}

	Style SelectStyleCore(IInspectable const&, DependencyObject const& container);

private:
	Style _defaultStyle{ nullptr };
	Style _clickableStyle{ nullptr };
};

struct SettingsExpander : SettingsExpander_base<SettingsExpander> {
	SettingsExpander();

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
	static DependencyProperty ItemContainerStyleSelectorProperty() { return _itemContainerStyleSelectorProperty; }

	IInspectable Header() const { return GetValue(_headerProperty); }
	void Header(IInspectable const& value) const { SetValue(_headerProperty, value); }

	IInspectable Description() const { return GetValue(_descriptionProperty); }
	void Description(IInspectable const& value) const { SetValue(_descriptionProperty, value); }

	Controls::IconElement HeaderIcon() const { return GetValue(_headerIconProperty).as<Controls::IconElement>(); }
	void HeaderIcon(Controls::IconElement const& value)const { SetValue(_headerIconProperty, value); }

	IInspectable Content() const { return GetValue(_contentProperty); }
	void Content(IInspectable const& value) const { SetValue(_contentProperty, value); }

	UIElement ItemsHeader() const { return GetValue(_itemsHeaderProperty).as<UIElement>(); }
	void ItemsHeader(UIElement const& value) const { SetValue(_itemsHeaderProperty, value); }

	UIElement ItemsFooter() const { return GetValue(_itemsFooterProperty).as<UIElement>(); }
	void ItemsFooter(UIElement const& value) const { SetValue(_itemsFooterProperty, value); }

	bool IsExpanded() const { return GetValue(_isExpandedProperty).as<bool>(); }
	void IsExpanded(bool value) const { SetValue(_isExpandedProperty, box_value(value)); }

	event_token Expanded(SignalDelegate const& handler) { return _expandedEvent.add(handler); }
	void Expanded(winrt::event_token const& token) { _expandedEvent.remove(token); }

	event_token Collapsed(SignalDelegate const& handler) { return _collapsedEvent.add(handler); }
	void Collapsed(winrt::event_token const& token) { _collapsedEvent.remove(token); }

	IVector<IInspectable> Items() const { return GetValue(_itemsProperty).as<IVector<IInspectable>>(); }
	void Items(IVector<IInspectable> const& value) const { SetValue(_itemsProperty, value); }

	IInspectable ItemsSource() const { return GetValue(_itemsSourceProperty); }
	void ItemsSource(IInspectable const& value) const { SetValue(_itemsSourceProperty, value); }

	IInspectable ItemTemplate() const { return GetValue(_itemTemplateProperty); }
	void ItemTemplate(IInspectable const& value) const { SetValue(_itemTemplateProperty, value); }

	Controls::StyleSelector ItemContainerStyleSelector() const {
		return GetValue(_itemContainerStyleSelectorProperty).as<Controls::StyleSelector>();
	}
	void ItemContainerStyleSelector(Controls::StyleSelector const& value) const {
		SetValue(_itemContainerStyleSelectorProperty, value);
	}

	void OnApplyTemplate();

private:
	static const DependencyProperty _headerProperty;
	static const DependencyProperty _descriptionProperty;
	static const DependencyProperty _headerIconProperty;
	static const DependencyProperty _contentProperty;
	static const DependencyProperty _itemsHeaderProperty;
	static const DependencyProperty _itemsFooterProperty;
	static const DependencyProperty _isExpandedProperty;
	static const DependencyProperty _itemsProperty;
	static const DependencyProperty _itemsSourceProperty;
	static const DependencyProperty _itemTemplateProperty;
	static const DependencyProperty _itemContainerStyleSelectorProperty;

	static void _OnIsExpandedChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args);
	static void _OnItemsConnectedPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _OnItemsConnectedPropertyChanged();

	event<SignalDelegate> _expandedEvent;
	event<SignalDelegate> _collapsedEvent;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsExpanderItemStyleSelector : SettingsExpanderItemStyleSelectorT<SettingsExpanderItemStyleSelector, implementation::SettingsExpanderItemStyleSelector> {
};

struct SettingsExpander : SettingsExpanderT<SettingsExpander, implementation::SettingsExpander> {
};

}
