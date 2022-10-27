#include "pch.h"
#include "AboutPage.h"
#if __has_include("AboutPage.g.cpp")
#include "AboutPage.g.cpp"
#endif


namespace winrt::Magpie::UI::implementation {

AboutPage::AboutPage() {
	InitializeComponent();
}

static void OpenUrl(const wchar_t* url) noexcept {
	ShellExecute(NULL, L"open", url, nullptr, nullptr, SW_NORMAL);
}

void AboutPage::BugReportButton_Click(IInspectable const&, RoutedEventArgs const&) {
	OpenUrl(L"https://github.com/Blinue/Magpie/issues/new?assignees=&labels=bug&template=01_bug.yaml");
}

void AboutPage::FeatureRequestButton_Click(IInspectable const&, RoutedEventArgs const&) {
	OpenUrl(L"https://github.com/Blinue/Magpie/issues/new?assignees=&labels=enhancement&template=03_request.yaml");
}

void AboutPage::DiscussionsButton_Click(IInspectable const&, RoutedEventArgs const&) {
	OpenUrl(L"https://github.com/Blinue/Magpie/discussions");
}

}
