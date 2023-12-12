// 移植自 https://github.com/CommunityToolkit/Windows/tree/bef863ca70bb1edf8c940198dd5cc74afa5d2aab/components/SettingsControls/src/SettingsExpander

#include "pch.h"
#include "SettingsExpander.h"
#if __has_include("SettingsExpanderItemStyleSelector.g.cpp")
#include "SettingsExpanderItemStyleSelector.g.cpp"
#endif
#if __has_include("SettingsExpander.g.cpp")
#include "SettingsExpander.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;

namespace winrt::Magpie::App::implementation {

Style SettingsExpanderItemStyleSelector::SelectStyleCore(IInspectable const&, DependencyObject const& container) {
	if (SettingsCard settingsCard = container.try_as<SettingsCard>()) {
		if (settingsCard.IsClickEnabled()) {
			return _clickableStyle;
		}
	}

	return _defaultStyle;
}

static constexpr const wchar_t* PART_ItemsListView = L"PART_ItemsListView";

const DependencyProperty SettingsExpander::_headerProperty = DependencyProperty::Register(
	L"Header",
	xaml_typename<IInspectable>(),
	xaml_typename<class_type>(),
	nullptr
);

const DependencyProperty SettingsExpander::_descriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<class_type>(),
	nullptr
);

const DependencyProperty SettingsExpander::_headerIconProperty = DependencyProperty::Register(
	L"HeaderIcon",
	xaml_typename<IconElement>(),
	xaml_typename<class_type>(),
	nullptr
);

const DependencyProperty SettingsExpander::_contentProperty = DependencyProperty::Register(
	L"Content",
	xaml_typename<IInspectable>(),
	xaml_typename<class_type>(),
	nullptr
);

const DependencyProperty SettingsExpander::_itemsHeaderProperty = DependencyProperty::Register(
	L"ItemsHeader",
	xaml_typename<UIElement>(),
	xaml_typename<class_type>(),
	nullptr
);

const DependencyProperty SettingsExpander::_itemsFooterProperty = DependencyProperty::Register(
	L"ItemsFooter",
	xaml_typename<UIElement>(),
	xaml_typename<class_type>(),
	nullptr
);

const DependencyProperty SettingsExpander::_isExpandedProperty = DependencyProperty::Register(
	L"IsExpanded",
	xaml_typename<bool>(),
	xaml_typename<class_type>(),
	PropertyMetadata(box_value(false), &SettingsExpander::_OnIsExpandedChanged)
);

const DependencyProperty SettingsExpander::_itemsProperty = DependencyProperty::Register(
	L"Items",
	xaml_typename<IVector<IInspectable>>(),
	xaml_typename<class_type>(),
	PropertyMetadata(nullptr, &SettingsExpander::_OnItemsConnectedPropertyChanged)
);

const DependencyProperty SettingsExpander::_itemsSourceProperty = DependencyProperty::Register(
	L"ItemsSource",
	xaml_typename<IInspectable>(),
	xaml_typename<class_type>(),
	PropertyMetadata(nullptr, &SettingsExpander::_OnItemsConnectedPropertyChanged)
);

const DependencyProperty SettingsExpander::_itemTemplateProperty = DependencyProperty::Register(
	L"ItemTemplate",
	xaml_typename<IInspectable>(),
	xaml_typename<class_type>(),
	nullptr
);

const DependencyProperty SettingsExpander::_itemContainerStyleSelectorProperty = DependencyProperty::Register(
	L"ItemContainerStyleSelector",
	xaml_typename<StyleSelector>(),
	xaml_typename<class_type>(),
	nullptr
);

const DependencyProperty SettingsExpander::_canReorderItemsProperty = DependencyProperty::Register(
	L"CanReorderItems",
	xaml_typename<bool>(),
	xaml_typename<class_type>(),
	PropertyMetadata(box_value(false))
);

SettingsExpander::SettingsExpander() {
	DefaultStyleKey(box_value(GetRuntimeClassName()));
	Items(single_threaded_vector<IInspectable>());
}

void SettingsExpander::OnApplyTemplate() {
	base_type::OnApplyTemplate();
	_OnItemsConnectedPropertyChanged();
}

void SettingsExpander::_OnIsExpandedChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args) {
	SettingsExpander* that = get_self<SettingsExpander>(sender.as<class_type>());

	if (args.NewValue().as<bool>()) {
		that->_expandedEvent();
	} else {
		that->_collapsedEvent();
	}
}

void SettingsExpander::_OnItemsConnectedPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsExpander>(sender.as<class_type>())->_OnItemsConnectedPropertyChanged();
}

void SettingsExpander::_OnItemsConnectedPropertyChanged() {
	ListView listView = GetTemplateChild(PART_ItemsListView).as<ListView>();
	if (!listView) {
		return;
	}

	IInspectable datasource = ItemsSource();
	listView.ItemsSource(datasource ? datasource : Items());

	// 应用样式
	StyleSelector styleSelector = ItemContainerStyleSelector();
	for (IInspectable item : listView.Items()) {
		SettingsCard element = item.try_as<SettingsCard>();
		if (!element) {
			continue;
		}
		
		if (element.ReadLocalValue(FrameworkElement::StyleProperty()) == DependencyProperty::UnsetValue()) {
			element.Style(styleSelector.SelectStyle(nullptr, element));
		}
	}
}

}
