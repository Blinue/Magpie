#include "pch.h"
#include "ScalingModesPage.h"
#if __has_include("ScalingModesPage.g.cpp")
#include "ScalingModesPage.g.cpp"
#endif
#include "ComboBoxHelper.h"
#include "EffectsService.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;


namespace winrt::Magpie::UI::implementation {

ScalingModesPage::ScalingModesPage() {
	InitializeComponent();

	_BuildEffectMenu();
}

void ScalingModesPage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

void ScalingModesPage::AddEffectButton_Click(IInspectable const&, RoutedEventArgs const&) {
	AddEffectMenuFlyout().ShowAt(AddEffectButton());
}

void ScalingModesPage::_BuildEffectMenu() noexcept {
	std::vector<MenuFlyoutItemBase> rootItems;

	std::unordered_map<std::wstring_view, MenuFlyoutSubItem> folders;
	for (const auto& effect : EffectsService::Get().Effects()) {
		std::wstring_view name(effect.name);

		size_t delimPos = name.find_last_of(L'\\');
		if (delimPos == std::wstring::npos) {
			MenuFlyoutItem item;
			item.Text(name);
			rootItems.emplace_back(std::move(item));
			continue;
		}

		MenuFlyoutItem item;
		item.Text(name.substr(delimPos + 1));

		std::wstring_view dir = name.substr(0, delimPos);
		auto it = folders.find(dir);
		if (it != folders.end()) {
			it->second.Items().Append(item);
		} else {
			MenuFlyoutSubItem folder;
			folder.Text(hstring(dir));
			folder.Items().Append(item);

			rootItems.push_back(folder);
			folders.emplace(dir, folder);
		}
	}

	std::sort(rootItems.begin(), rootItems.end(), [](MenuFlyoutItemBase const& l, MenuFlyoutItemBase const& r) {
		bool isLSubMenu = get_class_name(l) == name_of<MenuFlyoutSubItem>();
		bool isRSubMenu = get_class_name(r) == name_of<MenuFlyoutSubItem>();

		if (isLSubMenu != isRSubMenu) {
			return isLSubMenu;
		}

		if (isLSubMenu) {
			return l.as<MenuFlyoutSubItem>().Text() < r.as<MenuFlyoutSubItem>().Text();
		} else {
			return l.as<MenuFlyoutItem>().Text() < r.as<MenuFlyoutItem>().Text();
		}
	});

	for (MenuFlyoutItemBase& item : rootItems) {
		AddEffectMenuFlyout().Items().Append(std::move(item));
	}
}

}
