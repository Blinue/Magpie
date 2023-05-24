#include "pch.h"
#include "CaptionButtonsControl.h"
#if __has_include("CaptionButtonsControl.g.cpp")
#include "CaptionButtonsControl.g.cpp"
#endif

namespace winrt::Magpie::App::implementation {

void CaptionButtonsControl::CloseButton_Click(IInspectable const&, RoutedEventArgs const&) noexcept {
	Application::Current().as<App>().Quit();
}

}
