#include "pch.h"
#include "AboutPage.h"
#if __has_include("AboutPage.g.cpp")
#include "AboutPage.g.cpp"
#endif


namespace winrt::Magpie::UI::implementation {

AboutPage::AboutPage() {
	InitializeComponent();
}

void AboutPage::BugReportButton_Click(IInspectable const&, RoutedEventArgs const&) {
	ShellExecute(NULL, L"open", L"https://github.com/Blinue/Magpie/issues/new?assignees=&labels=bug&template=01_bug.yaml", nullptr, nullptr, SW_NORMAL);
}

void AboutPage::FeatureRequestButton_Click(IInspectable const&, RoutedEventArgs const&) {
	ShellExecute(NULL, L"open", L"https://github.com/Blinue/Magpie/issues/new?assignees=&labels=enhancement&template=03_request.yaml", nullptr, nullptr, SW_NORMAL);
}

}
