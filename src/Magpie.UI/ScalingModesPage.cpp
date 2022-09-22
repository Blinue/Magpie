#include "pch.h"
#include "ScalingModesPage.h"
#if __has_include("ScalingModesPage.g.cpp")
#include "ScalingModesPage.g.cpp"
#endif
#include "ComboBoxHelper.h"
#include "EffectsService.h"
#include "PageHelper.h"


using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Input;


namespace winrt::Magpie::UI::implementation {

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

void ScalingModesPage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ComboBoxHelper::DropDownOpened(*this, sender);
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

void ScalingModesPage::_BuildEffectMenu() noexcept {
	std::vector<MenuFlyoutItemBase> rootItems;

	std::unordered_map<std::wstring_view, MenuFlyoutSubItem> folders;
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
