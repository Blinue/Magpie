// 移植自 https://github.com/CommunityToolkit/Windows/tree/efbaf965029806fe29e02a6421af3c8f434e1460/components/SettingsControls/src/SettingsExpander

#include "pch.h"
#include "SettingsExpander.h"
#if __has_include("SettingsExpander.g.cpp")
#include "SettingsExpander.g.cpp"
#endif
#if __has_include("SettingsExpanderStyle.g.cpp")
#include "SettingsExpanderStyle.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;

namespace winrt::Magpie::implementation {

static constexpr const wchar_t* PART_ItemsContainer = L"PART_ItemsContainer";
static constexpr const wchar_t* PART_ExpanderHeader = L"PART_ExpanderHeader";
static constexpr const wchar_t* PART_ExpandCollapseChevron = L"PART_ExpandCollapseChevron";

DependencyProperty SettingsExpander::_headerProperty{ nullptr };
DependencyProperty SettingsExpander::_descriptionProperty{ nullptr };
DependencyProperty SettingsExpander::_headerIconProperty{ nullptr };
DependencyProperty SettingsExpander::_contentProperty{ nullptr };
DependencyProperty SettingsExpander::_itemsHeaderProperty{ nullptr };
DependencyProperty SettingsExpander::_itemsFooterProperty{ nullptr };
DependencyProperty SettingsExpander::_isExpandedProperty{ nullptr };
DependencyProperty SettingsExpander::_itemsProperty{ nullptr };
DependencyProperty SettingsExpander::_itemsSourceProperty{ nullptr };
DependencyProperty SettingsExpander::_itemTemplateProperty{ nullptr };

SettingsExpander::SettingsExpander() {
	DefaultStyleKey(box_value(GetRuntimeClassName()));
	Items(single_threaded_vector<IInspectable>());
}

void SettingsExpander::RegisterDependencyProperties() {
	_headerProperty = DependencyProperty::Register(
		L"Header",
		xaml_typename<IInspectable>(),
		xaml_typename<class_type>(),
		nullptr
	);

	_descriptionProperty = DependencyProperty::Register(
		L"Description",
		xaml_typename<IInspectable>(),
		xaml_typename<class_type>(),
		nullptr
	);

	_headerIconProperty = DependencyProperty::Register(
		L"HeaderIcon",
		xaml_typename<IconElement>(),
		xaml_typename<class_type>(),
		nullptr
	);

	_contentProperty = DependencyProperty::Register(
		L"Content",
		xaml_typename<IInspectable>(),
		xaml_typename<class_type>(),
		nullptr
	);

	_itemsHeaderProperty = DependencyProperty::Register(
		L"ItemsHeader",
		xaml_typename<UIElement>(),
		xaml_typename<class_type>(),
		nullptr
	);

	_itemsFooterProperty = DependencyProperty::Register(
		L"ItemsFooter",
		xaml_typename<UIElement>(),
		xaml_typename<class_type>(),
		nullptr
	);

	_isExpandedProperty = DependencyProperty::Register(
		L"IsExpanded",
		xaml_typename<bool>(),
		xaml_typename<class_type>(),
		PropertyMetadata(box_value(false), &SettingsExpander::_OnIsExpandedChanged)
	);

	_itemsProperty = DependencyProperty::Register(
		L"Items",
		xaml_typename<IVector<IInspectable>>(),
		xaml_typename<class_type>(),
		PropertyMetadata(nullptr, &SettingsExpander::_OnItemsConnectedPropertyChanged)
	);

	_itemsSourceProperty = DependencyProperty::Register(
		L"ItemsSource",
		xaml_typename<IInspectable>(),
		xaml_typename<class_type>(),
		PropertyMetadata(nullptr, &SettingsExpander::_OnItemsConnectedPropertyChanged)
	);

	_itemTemplateProperty = DependencyProperty::Register(
		L"ItemTemplate",
		xaml_typename<IInspectable>(),
		xaml_typename<class_type>(),
		nullptr
	);
}

void SettingsExpander::OnApplyTemplate() {
	base_type::OnApplyTemplate();

	_OnItemsConnectedPropertyChanged();

	auto expander = VisualTreeHelper::GetChild(*this, 0).try_as<MUXC::Expander>();
	expander.ApplyTemplate();

	// 跳过动画
	auto expanderRoot = VisualTreeHelper::GetChild(expander, 0).try_as<Grid>();
	for (VisualStateGroup group : VisualStateManager::GetVisualStateGroups(expanderRoot)) {
		for (VisualState state : group.States()) {
			state.Storyboard().SkipToFill();
		}
	}

	auto header = expander.try_as<IControlProtected>()
		.GetTemplateChild(PART_ExpanderHeader)
		.try_as<Primitives::ToggleButton>();
	header.ApplyTemplate();
	_expandCollapseChevron = header.try_as<IControlProtected>()
		.GetTemplateChild(PART_ExpandCollapseChevron)
		.try_as<MUXC::AnimatedIcon>();
	_UpdateAnimatedIcon();
}

void SettingsExpander::_OnIsExpandedChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args) {
	SettingsExpander* that = get_self<SettingsExpander>(sender.as<class_type>());

	if (args.NewValue().as<bool>()) {
		that->Expanded.Invoke();
	} else {
		that->Collapsed.Invoke();
	}

	if (that->_expandCollapseChevron) {
		that->_UpdateAnimatedIcon();
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

// 防止加载后立刻展示动画
void SettingsExpander::_UpdateAnimatedIcon() {
	MUXC::AnimatedIcon::SetState(_expandCollapseChevron, IsExpanded() ? L"PointerOverOn" : L"PointerOverOff");
}

}
