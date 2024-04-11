#include "pch.h"
#include "TitleBarControl.h"
#if __has_include("TitleBarControl.g.cpp")
#include "TitleBarControl.g.cpp"
#endif
#include "IconHelper.h"
#include "Win32Utils.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Media::Imaging;

namespace winrt::Magpie::App::implementation {

TitleBarControl::TitleBarControl() {
	// 异步加载 Logo
	[](TitleBarControl* that)->fire_and_forget {
		auto weakThis = that->get_weak();

		SoftwareBitmapSource bitmap;
		co_await bitmap.SetBitmapAsync(IconHelper::ExtractIconFromExe(
			Win32Utils::GetExePath().c_str(), 40, USER_DEFAULT_SCREEN_DPI));

		if (!weakThis.get()) {
			co_return;
		}

		that->_logo = std::move(bitmap);
		that->RaisePropertyChanged(L"Logo");
	}(this);
}

void TitleBarControl::Loading(FrameworkElement const&, IInspectable const&) {
	MUXC::NavigationView rootNavigationView = Application::Current().as<App>().RootPage().RootNavigationView();
	rootNavigationView.DisplayModeChanged([this](const auto&, const auto& args) {
		bool expanded = args.DisplayMode() == MUXC::NavigationViewDisplayMode::Expanded;
		VisualStateManager::GoToState(*this, expanded ? L"Expanded" : L"Compact", true);
	});
}

void TitleBarControl::IsWindowActive(bool value) {
	VisualStateManager::GoToState(*this, value ? L"Active" : L"NotActive", false);
	CaptionButtons().IsWindowActive(value);
}

}
