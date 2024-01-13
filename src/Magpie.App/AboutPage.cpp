#include "pch.h"
#include "AboutPage.h"
#if __has_include("AboutPage.g.cpp")
#include "AboutPage.g.cpp"
#endif
#include "Win32Utils.h"
#include "CommonSharedConstants.h"

namespace winrt::Magpie::App::implementation {

void AboutPage::VersionTextBlock_DoubleTapped(IInspectable const&, Input::DoubleTappedRoutedEventArgs const&) {
	// 按住 Alt 键双击版本号即可启用开发者模式
	if (!_viewModel.IsDeveloperMode() && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
		_viewModel.IsDeveloperMode(true);
		
		const hstring message = ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID)
			.GetString(L"About_DeveloperModeEnabled");
		Application::Current().as<App>().RootPage().ShowToast(message);
	}
}

void AboutPage::BugReportButton_Click(IInspectable const&, RoutedEventArgs const&) {
	Win32Utils::ShellOpen(L"https://github.com/Blinue/Magpie/issues/new?assignees=&labels=bug&template=01_bug.yaml");
}

void AboutPage::FeatureRequestButton_Click(IInspectable const&, RoutedEventArgs const&) {
	Win32Utils::ShellOpen(L"https://github.com/Blinue/Magpie/issues/new?assignees=&labels=enhancement&template=03_request.yaml");
}

void AboutPage::DiscussionsButton_Click(IInspectable const&, RoutedEventArgs const&) {
	Win32Utils::ShellOpen(L"https://github.com/Blinue/Magpie/discussions");
}

}
