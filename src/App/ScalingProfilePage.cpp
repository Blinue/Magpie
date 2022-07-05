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


using namespace winrt;
using namespace Windows::Globalization::NumberFormatting;

namespace winrt::Magpie::App::implementation {

ScalingProfilePage::ScalingProfilePage() {
	InitializeComponent();

	if (Win32Utils::GetOSBuild() < 22000) {
		// Segoe MDL2 Assets 不存在 Move 图标
		AdjustCursorSpeedFontIcon().Glyph(L"\uE962");
	}

	if (GetSystemMetrics(SM_CMONITORS) <= 1) {
		// 只有一个显示器时隐藏多显示器选项
		MultiMonitorSettingItem().Visibility(Visibility::Collapsed);
		Is3DGameModeSettingItem().Margin({ 0,0,0,-2 });
	}

	IncrementNumberRounder rounder;
	// 保留一位小数
	// 不知为何不能在 XAML 中设置
	rounder.Increment(0.1);
	_numberFormatter.NumberRounder(rounder);
	_numberFormatter.FractionDigits(0);
}

void ScalingProfilePage::OnNavigatedTo(Navigation::NavigationEventArgs const& args) {
	uint32_t profileId = args.Parameter().as<uint32_t>();
	_viewModel = make<ScalingProfileViewModel>(profileId);

	if (_viewModel.GraphicsAdapters().Size() <= 2) {
		// 只有一个显卡时隐藏显示卡选项
		GraphicsAdapterSettingItem().Visibility(Visibility::Collapsed);
		ShowFPSSettingItem().Margin({ 0,-2,0,0 });
	}
}

void ScalingProfilePage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

}
