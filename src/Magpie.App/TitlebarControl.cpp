#include "pch.h"
#include "TitleBarControl.h"
#if __has_include("TitleBarControl.g.cpp")
#include "TitleBarControl.g.cpp"
#endif
#include "IconHelper.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Media::Imaging;

namespace winrt::Magpie::App::implementation {

TitleBarControl::TitleBarControl() {
	// 异步加载 Logo
	[](TitleBarControl* that)->fire_and_forget {
		wchar_t exePath[MAX_PATH];
		GetModuleFileName(NULL, exePath, MAX_PATH);

		auto weakThis = that->get_weak();

		SoftwareBitmapSource bitmap;
		co_await bitmap.SetBitmapAsync(IconHelper::ExtractIconFromExe(exePath, 40, USER_DEFAULT_SCREEN_DPI));

		if (!weakThis.get()) {
			co_return;
		}

		that->_logo = std::move(bitmap);
		that->_propertyChangedEvent(*that, PropertyChangedEventArgs(L"Logo"));
	}(this);
}

void TitleBarControl::Loading(FrameworkElement const&, IInspectable const&) {
	MUXC::NavigationView rootNavigationView = Application::Current().as<App>().MainPage().RootNavigationView();
	rootNavigationView.DisplayModeChanged([this](const auto&, const auto& args) {
		bool expanded = args.DisplayMode() == MUXC::NavigationViewDisplayMode::Expanded;
		VisualStateManager::GoToState(*this, expanded ? L"Expanded" : L"Compact", true);
	});
}

}
