#include "pch.h"
#include "TitleBarControl.h"
#if __has_include("TitleBarControl.g.cpp")
#include "TitleBarControl.g.cpp"
#endif
#include "IconHelper.h"
#include "Win32Helper.h"
#include "App.h"
#include "CaptionButtonsControl.h"
#include "RootPage.h"

using namespace ::Magpie;
using namespace winrt;
using namespace Windows::UI::Xaml::Media::Imaging;

namespace winrt::Magpie::implementation {

TitleBarControl::TitleBarControl() {
	// 异步加载 Logo
	[](TitleBarControl* that)->fire_and_forget {
		auto weakThis = that->get_weak();

		SoftwareBitmapSource bitmap;
		co_await bitmap.SetBitmapAsync(IconHelper::ExtractAppSmallIcon());

		if (!weakThis.get()) {
			co_return;
		}

		that->_logo = std::move(bitmap);
		that->RaisePropertyChanged(L"Logo");
	}(this);
}

void TitleBarControl::TitleBarControl_Loading(FrameworkElement const&, IInspectable const&) {
	MUXC::NavigationView rootNavigationView = App::Get().RootPage()->RootNavigationView();
	rootNavigationView.DisplayModeChanged([this](const auto&, const auto& args) {
		bool expanded = args.DisplayMode() == MUXC::NavigationViewDisplayMode::Expanded;
		VisualStateManager::GoToState(
			*this, expanded ? L"Expanded" : L"Compact", App::Get().RootPage()->IsLoaded());
		LeftBottomPointChanged.Invoke();
	});
}

void TitleBarControl::IsWindowActive(bool value) {
	VisualStateManager::GoToState(*this, value ? L"Active" : L"NotActive", false);
	CaptionButtons().IsWindowActive(value);
}

CaptionButtonsControl& TitleBarControl::CaptionButtons() noexcept {
	return *get_self<CaptionButtonsControl>(TitleBarControlT::CaptionButtons());
}

Point TitleBarControl::LeftBottomPoint() noexcept {
	const auto& rootPage = App::Get().RootPage();
	bool expanded = rootPage->RootNavigationView().DisplayMode() == MUXC::NavigationViewDisplayMode::Expanded;
	// 左边界不包含 RootStackPanel 的 Margin。Margin 属性在动画播放结束才会改变，不要使用。
	return TransformToVisual(*rootPage).TransformPoint({ expanded ? 0.0f : 46.0f, (float)ActualHeight()});
}

}
