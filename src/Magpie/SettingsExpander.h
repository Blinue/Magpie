#pragma once
#include "SettingsExpander.g.h"
#include "Event.h"

namespace winrt::Magpie::implementation {

struct SettingsExpander : SettingsExpanderT<SettingsExpander> {
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, Header, _headerProperty)
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, Description, _descriptionProperty)
	DEFINE_DEPENDENCY_PROPERTY(IconElement, HeaderIcon, _headerIconProperty)
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, Content, _contentProperty)
	DEFINE_DEPENDENCY_PROPERTY(UIElement, ItemsHeader, _itemsHeaderProperty)
	DEFINE_DEPENDENCY_PROPERTY(UIElement, ItemsFooter, _itemsFooterProperty)
	DEFINE_DEPENDENCY_PROPERTY(bool, IsExpanded, _isExpandedProperty)
	DEFINE_DEPENDENCY_PROPERTY(IVector<IInspectable>, Items, _itemsProperty)
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, ItemsSource, _itemsSourceProperty)
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, ItemTemplate, _itemTemplateProperty)
	DEFINE_DEPENDENCY_PROPERTY(bool, IsWrapEnabled, _isWrapEnabledProperty)

public:
	SettingsExpander();

	void OnApplyTemplate();

	::Magpie::WinRTEvent<SignalDelegate> Expanded;
	::Magpie::WinRTEvent<SignalDelegate> Collapsed;

private:
	static void _RegisterDependencyProperties();

	static void _OnIsExpandedChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args);

	void _OnItemsConnectedPropertyChanged();

	void _UpdateAnimatedIcon();

	MUXC::AnimatedIcon _expandCollapseChevron{ nullptr };
};

}

BASIC_FACTORY(SettingsExpander)
