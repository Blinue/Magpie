#include "pch.h"
#include "CaptionButtonsControl.h"
#if __has_include("CaptionButtonsControl.g.cpp")
#include "CaptionButtonsControl.g.cpp"
#endif

namespace winrt::Magpie::App::implementation {

double CaptionButtonsControl::CaptionButtonWidth() const noexcept {
	return unbox_value<double>(Resources().Lookup(box_value(L"CaptionButtonWidth")));
}

void CaptionButtonsControl::CloseButton_Click(IInspectable const&, RoutedEventArgs const&) noexcept {
	Application::Current().as<App>().Quit();
}

}
