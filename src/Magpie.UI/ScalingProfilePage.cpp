#include "pch.h"
#include "ScalingProfilePage.h"
#if __has_include("ScalingProfilePage.g.cpp")
#include "ScalingProfilePage.g.cpp"
#endif
#include "ScalingProfileViewModel.h"
#include "Win32Utils.h"
#include "ComboBoxHelper.h"
#include "AppSettings.h"
#include "ScalingProfileService.h"
#include "ScalingProfile.h"
#include "PageHelper.h"


using namespace winrt;
using namespace Windows::Globalization::NumberFormatting;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Input;


namespace winrt::Magpie::UI::implementation {

ScalingProfilePage::ScalingProfilePage() {
	InitializeComponent();

	MainPage mainPage = Application::Current().as<App>().MainPage();
	_displayModeChangedRevoker = mainPage.RootNavigationView().DisplayModeChanged(
		auto_revoke,
		[&](auto const&, auto const&) { PageHelper::UpdateHeaderActionStyle(HeaderActionStackPanel()); }
	);
	PageHelper::UpdateHeaderActionStyle(HeaderActionStackPanel());

	if (!Win32Utils::GetOSVersion().IsWin11()) {
		// Segoe MDL2 Assets 不存在 Move 图标
		AdjustCursorSpeedFontIcon().Glyph(L"\uE962");
	}

	if (GetSystemMetrics(SM_CMONITORS) <= 1) {
		// 只有一个显示器时隐藏多显示器选项
		MultiMonitorSettingsCard().Visibility(Visibility::Collapsed);
		Is3DGameModeSettingsCard().Margin({ 0,0,0,-2 });
	}
}

void ScalingProfilePage::OnNavigatedTo(Navigation::NavigationEventArgs const& args) {
	int32_t profileIdx = args.Parameter().as<int32_t>();
	_viewModel = make<ScalingProfileViewModel>(profileIdx);

	if (_viewModel.GraphicsAdapters().Size() <= 2) {
		// 只有一个显卡时隐藏显示卡选项
		GraphicsAdapterSettingsCard().Visibility(Visibility::Collapsed);
		ShowFPSSettingsCard().Margin({ 0,-2,0,0 });
	}
}

void ScalingProfilePage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

void ScalingProfilePage::CursorScalingComboBox_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&) {
	if ((CursorScaling)_viewModel.CursorScaling() == CursorScaling::Custom) {
		CursorScalingComboBox().MinWidth(0);
		CustomCursorScalingNumberBox().Visibility(Visibility::Visible);
		CustomCursorScalingLabel().Visibility(Visibility::Visible);
	} else {
		double minWidth = Application::Current().Resources()
			.Lookup(box_value(L"SettingBoxMinWidth"))
			.as<double>();
		CursorScalingComboBox().MinWidth(minWidth);
		CustomCursorScalingNumberBox().Visibility(Visibility::Collapsed);
		CustomCursorScalingLabel().Visibility(Visibility::Collapsed);
	}
}

INumberFormatter2 ScalingProfilePage::NumberFormatter() noexcept {
	static DecimalFormatter numberFormatter = []() {
		DecimalFormatter result;
		IncrementNumberRounder rounder;
		// 保留五位小数
		rounder.Increment(0.1);
		result.NumberRounder(rounder);
		result.FractionDigits(0);
		return result;
	}();
	
	return numberFormatter;
}

void ScalingProfilePage::RenameMenuItem_Click(IInspectable const&, RoutedEventArgs const&) {
	RenameFlyout().ShowAt(MoreOptionsButton());
}

void ScalingProfilePage::RenameFlyout_Opening(IInspectable const&, IInspectable const&) {
	TextBox tb = RenameTextBox();
	hstring name = _viewModel.Name();
	tb.Text(name);
	tb.SelectionStart(name.size());
}

void ScalingProfilePage::RenameConfirmButton_Click(IInspectable const&, RoutedEventArgs const&) {
	RenameFlyout().Hide();
	_viewModel.Rename();
}

void ScalingProfilePage::RenameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args) {
	if (args.Key() == VirtualKey::Enter && _viewModel.IsRenameConfirmButtonEnabled()) {
		RenameConfirmButton_Click(nullptr, nullptr);
	}
}

void ScalingProfilePage::ReorderMenuItem_Click(IInspectable const&, RoutedEventArgs const&) {
	ReorderFlyout().ShowAt(MoreOptionsButton());
}

void ScalingProfilePage::DeleteMenuItem_Click(IInspectable const&, RoutedEventArgs const&) {
	DeleteFlyout().ShowAt(MoreOptionsButton());
}

void ScalingProfilePage::DeleteButton_Click(IInspectable const&, RoutedEventArgs const&) {
	DeleteFlyout().Hide();
	_viewModel.Delete();
}

}
