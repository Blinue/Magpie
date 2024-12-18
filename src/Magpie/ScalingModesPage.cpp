#include "pch.h"
#include "ScalingModesPage.h"
#if __has_include("ScalingModesPage.g.cpp")
#include "ScalingModesPage.g.cpp"
#endif
#if __has_include("ScalingType.g.cpp")
#include "ScalingType.g.cpp"
#endif
#include "ComboBoxHelper.h"
#include "EffectsService.h"
#include <parallel_hashmap/phmap.h>

using namespace ::Magpie;
using namespace winrt;
using namespace Windows::Globalization::NumberFormatting;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Input;

namespace winrt::Magpie::implementation {

ScalingModesPage::ScalingModesPage() {
	_BuildEffectMenu();
}

INumberFormatter2 ScalingModesPage::NumberFormatter() noexcept {
	static DecimalFormatter numberFormatter = []() {
		DecimalFormatter result;
		IncrementNumberRounder rounder;
		// 保留四位小数
		rounder.Increment(0.0001);
		result.NumberRounder(rounder);
		result.FractionDigits(0);
		return result;
	}();

	return numberFormatter;
}

void ScalingModesPage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

void ScalingModesPage::EffectSettingsCard_Loaded(IInspectable const& sender, RoutedEventArgs const&) {
	XamlHelper::UpdateThemeOfTooltips(sender.as<DependencyObject>(), ActualTheme());
}

void ScalingModesPage::AddEffectButton_Click(IInspectable const& sender, RoutedEventArgs const&) {
	Button btn = sender.as<Button>();
	_curScalingMode = get_self<ScalingModeItem>(btn.Tag().as<winrt::Magpie::ScalingModeItem>());
	_addEffectMenuFlyout.ShowAt(btn);
}

void ScalingModesPage::NewScalingModeFlyout_Opening(IInspectable const&, IInspectable const&) {
	_viewModel->PrepareForAdd();
}

void ScalingModesPage::NewScalingModeNameTextBox_KeyDown(IInspectable const&, KeyRoutedEventArgs const& args) {
	if (args.Key() == VirtualKey::Enter) {
		if (_viewModel->IsAddButtonEnabled()) {
			NewScalingModeConfirmButton_Click(nullptr, nullptr);
		}
	}
}

void ScalingModesPage::NewScalingModeConfirmButton_Click(IInspectable const&, RoutedEventArgs const&) {
	NewScalingModeFlyout().Hide();
	_viewModel->AddScalingMode();
}

void ScalingModesPage::ScalingModeMoreOptionsButton_Click(IInspectable const& sender, RoutedEventArgs const&) {
	_moreOptionsButton = sender;
}

void ScalingModesPage::RemoveScalingModeMenuItem_Click(IInspectable const& sender, RoutedEventArgs const&) {
	MenuFlyoutItem menuItem = sender.as<MenuFlyoutItem>();
	ScalingModeItem* scalingModeItem = get_self<ScalingModeItem>(menuItem.Tag().as<winrt::Magpie::ScalingModeItem>());
	if (scalingModeItem->IsInUse()) {
		// 如果有缩放配置正在使用此缩放模式则弹出确认弹窗
		FlyoutBase::GetAttachedFlyout(menuItem)
			.ShowAt(_moreOptionsButton.as<FrameworkElement>());
	} else {
		scalingModeItem->Remove();
	}
}

void ScalingModesPage::_BuildEffectMenu() noexcept {
	std::vector<MenuFlyoutItemBase> rootItems;

	phmap::flat_hash_map<std::wstring_view, MenuFlyoutSubItem> folders;
	folders.reserve(13);
	for (const auto& effect : EffectsService::Get().Effects()) {
		std::wstring_view name(effect.name);

		MenuFlyoutItem item;
		item.Tag(box_value(effect.name));
		item.Click({ this, &ScalingModesPage::_AddEffectMenuFlyoutItem_Click });

		size_t delimPos = name.find_last_of(L'\\');
		if (delimPos == std::wstring::npos) {
			item.Text(name);
			rootItems.emplace_back(std::move(item));
			continue;
		}

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

	// 排序文件夹中的项目
	for (MenuFlyoutItemBase& item : rootItems) {
		MenuFlyoutSubItem folder = item.try_as<MenuFlyoutSubItem>();
		if (!folder) {
			break;
		}

		IVector<MenuFlyoutItemBase> items = folder.Items();
		// 读取到 std::vector 中以提高排序性能
		std::vector<MenuFlyoutItemBase> itemsVec(items.Size(), nullptr);
		items.GetMany(0, itemsVec);
		std::sort(itemsVec.begin(), itemsVec.end(), [](const MenuFlyoutItemBase& l, const MenuFlyoutItemBase& r) {
			hstring lEffectName = unbox_value<hstring>(l.as<MenuFlyoutItem>().Tag());
			hstring rEffectName = unbox_value<hstring>(r.as<MenuFlyoutItem>().Tag());

			const EffectInfo* lEffectInfo = EffectsService::Get().GetEffect(lEffectName);
			const EffectInfo* rEffectInfo = EffectsService::Get().GetEffect(rEffectName);

			return lEffectInfo->sortName < rEffectInfo->sortName;
		});
		items.ReplaceAll(itemsVec);
	}

	for (MenuFlyoutItemBase& item : rootItems) {
		_addEffectMenuFlyout.Items().Append(std::move(item));
	}
}

void ScalingModesPage::_AddEffectMenuFlyoutItem_Click(IInspectable const& sender, RoutedEventArgs const&) {
	hstring effectName = unbox_value<hstring>(sender.as<MenuFlyoutItem>().Tag());
	_curScalingMode->AddEffect(effectName);
}

}
