// 移植自 https://github.com/CommunityToolkit/Windows/tree/efbaf965029806fe29e02a6421af3c8f434e1460/components/SettingsControls/src/SettingsExpander

#include "pch.h"
#include "SettingsExpander.h"
#if __has_include("SettingsExpander.g.cpp")
#include "SettingsExpander.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;

namespace winrt::Magpie::implementation {

static constexpr const wchar_t* PART_ItemsContainer = L"PART_ItemsContainer";

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
		that->Expanded.Invoke();
	} else {
		that->Collapsed.Invoke();
	}
}

void SettingsExpander::_OnItemsConnectedPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsExpander>(sender.as<class_type>())->_OnItemsConnectedPropertyChanged();
}

void SettingsExpander::_OnItemsConnectedPropertyChanged() {
	ItemsControl itemsContainer = GetTemplateChild(PART_ItemsContainer).as<ItemsControl>();
	if (!itemsContainer) {
		return;
	}

	IInspectable datasource = ItemsSource();
	itemsContainer.ItemsSource(datasource ? datasource : Items());

	// 应用样式
	for (IInspectable const& item : itemsContainer.Items()) {
		SettingsCard settingsCard = item.try_as<SettingsCard>();
		if (!settingsCard) {
			continue;
		}
		
		if (settingsCard.ReadLocalValue(FrameworkElement::StyleProperty()) == DependencyProperty::UnsetValue()) {
			ResourceDictionary resources = Application::Current().Resources();
			const wchar_t* key = settingsCard.IsClickEnabled()
				? L"ClickableSettingsExpanderItemStyle"
				: L"DefaultSettingsExpanderItemStyle";
			settingsCard.Style(resources.Lookup(box_value(key)).as<Windows::UI::Xaml::Style>());
		}
	}
}

}
