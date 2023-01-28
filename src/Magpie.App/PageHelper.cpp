#include "pch.h"
#include "PageHelper.h"
#include "App.h"


using namespace winrt;
using namespace Windows::UI::Xaml::Controls;


namespace winrt::Magpie::App {

void PageHelper::UpdateHeaderActionStyle(StackPanel const& container) {
	MainPage mainPage = Application::Current().as<App>().MainPage();
	if (mainPage.RootNavigationView().DisplayMode() == MUXC::NavigationViewDisplayMode::Minimal) {
		container.Margin({ 0,2,0,-2 });
		container.Padding({ 0,-4,0,-4 });

		for (UIElement const& child : container.Children()) {
			Button btn = child.as<Button>();
			btn.Width(36);
			btn.Height(36);
		}
	} else {
		container.Margin({ 0,0,0,-3 });
		container.Padding({});

		for (UIElement const& child : container.Children()) {
			Button btn = child.as<Button>();
			btn.Width(40);
			btn.Height(40);
		}
	}
}

}
