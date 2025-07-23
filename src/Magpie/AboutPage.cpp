#include "pch.h"
#include "AboutPage.h"
#if __has_include("AboutPage.g.cpp")
#include "AboutPage.g.cpp"
#endif
#include "CommonSharedConstants.h"
#include "ToastService.h"
#include "Win32Helper.h"
#include "XamlHelper.h"

using namespace Magpie;

namespace winrt::Magpie::implementation {

void AboutPage::VersionTextBlock_DoubleTapped(IInspectable const&, Input::DoubleTappedRoutedEventArgs const&) {
	// 按住 Alt 键双击版本号即可启用开发者模式
	if (!_viewModel->IsDeveloperMode() && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
		_viewModel->IsDeveloperMode(true);
		
		const hstring message = ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID)
			.GetString(L"About_DeveloperModeEnabled");
		ToastService::Get().ShowMessageInApp({}, message);
	}
}

void AboutPage::BugReport_Click(IInspectable const&, RoutedEventArgs const&) {
	Win32Helper::ShellOpen(L"https://github.com/Blinue/Magpie/issues/new?assignees=&labels=bug&template=01_bug.yaml");
}

void AboutPage::FeatureRequest_Click(IInspectable const&, RoutedEventArgs const&) {
	Win32Helper::ShellOpen(L"https://github.com/Blinue/Magpie/issues/new?assignees=&labels=enhancement&template=03_request.yaml");
}

void AboutPage::Discussions_Click(IInspectable const&, RoutedEventArgs const&) {
	Win32Helper::ShellOpen(L"https://github.com/Blinue/Magpie/discussions");
}

void AboutPage::InfoBar_SizeChanged(IInspectable const& sender, SizeChangedEventArgs const&) const {
	// 修复 InfoBar 中 Tooltip 的主题
	XamlHelper::UpdateThemeOfTooltips(sender.try_as<DependencyObject>(), ActualTheme());
}

}
