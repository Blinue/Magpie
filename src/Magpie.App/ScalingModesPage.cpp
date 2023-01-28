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
#include "PageHelper.h"
#include <parallel_hashmap/phmap.h>

using namespace winrt;
using namespace Windows::Globalization::NumberFormatting;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Input;


namespace winrt::Magpie::App::implementation {

ScalingModesPage::ScalingModesPage() {
	InitializeComponent();

	MainPage mainPage = Application::Current().as<App>().MainPage();
	_displayModeChangedRevoker = mainPage.RootNavigationView().DisplayModeChanged(
		auto_revoke,
		[&](auto const&, auto const&) { PageHelper::UpdateHeaderActionStyle(HeaderActionStackPanel()); }
	);
	PageHelper::UpdateHeaderActionStyle(HeaderActionStackPanel());

	_BuildEffectMenu();
}

IVector<IInspectable> ScalingModesPage::ScalingTypes() noexcept {
	static IVector<IInspectable> types = single_threaded_vector(std::vector<IInspectable>{
		Magpie::App::ScalingType(L"倍数", L"指定相对于输入图像的缩放倍数"),
		Magpie::App::ScalingType(L"适应", L"指定等比缩放到充满屏幕后的缩放倍数"),
		Magpie::App::ScalingType(L"绝对", L"指定缩放后的尺寸"),
		Magpie::App::ScalingType(L"填充", L"充满屏幕，画面可能被拉伸")
	});

	return types;
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
	XamlUtils::UpdateThemeOfTooltips(sender.as<DependencyObject>(), ActualTheme());
}

void ScalingModesPage::AddEffectButton_Click(IInspectable const& sender, RoutedEventArgs const&) {
	Button btn = sender.as<Button>();
	_curScalingMode = btn.Tag().as<ScalingModeItem>();
	_addEffectMenuFlyout.ShowAt(btn);
}

void ScalingModesPage::NewScalingModeFlyout_Opening(IInspectable const&, IInspectable const&) {
	_viewModel.PrepareForAdd();
}

void ScalingModesPage::NewScalingModeNameTextBox_KeyDown(IInspectable const&, KeyRoutedEventArgs const& args) {
	if (args.Key() == VirtualKey::Enter) {
		if (_viewModel.IsAddButtonEnabled()) {
			NewScalingModeConfirmButton_Click(nullptr, nullptr);
		}
	}
}

void ScalingModesPage::NewScalingModeConfirmButton_Click(IInspectable const&, RoutedEventArgs const&) {
	NewScalingModeFlyout().Hide();
	_viewModel.AddScalingMode();
}

void ScalingModesPage::ScalingModeMoreOptionsButton_Click(IInspectable const& sender, RoutedEventArgs const&) {
	_moreOptionsButton = sender;
}

void ScalingModesPage::RemoveScalingModeMenuItem_Click(IInspectable const& sender, RoutedEventArgs const&) {
	MenuFlyoutItem menuItem = sender.as<MenuFlyoutItem>();
	ScalingModeItem scalingModeItem = menuItem.Tag().as<ScalingModeItem>();
	if (scalingModeItem.IsInUse()) {
		// 如果有缩放配置正在使用此缩放模式则弹出确认弹窗
		FlyoutBase::GetAttachedFlyout(menuItem)
			.ShowAt(_moreOptionsButton.as<FrameworkElement>());
	} else {
		scalingModeItem.Remove();
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

	for (MenuFlyoutItemBase& item : rootItems) {
		_addEffectMenuFlyout.Items().Append(std::move(item));
	}
}

void ScalingModesPage::_AddEffectMenuFlyoutItem_Click(IInspectable const& sender, RoutedEventArgs const&) {
	hstring effectName = unbox_value<hstring>(sender.as<MenuFlyoutItem>().Tag());
	_curScalingMode.AddEffect(effectName);
}

}
