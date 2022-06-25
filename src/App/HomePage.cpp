#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Controls::Primitives;


namespace winrt::Magpie::App::implementation {

HomePage::HomePage() {
	InitializeComponent();

	App app = Application::Current().as<App>();
	_settings = app.Settings();

	AutoRestoreToggleSwitch().IsOn(_settings.IsAutoRestore());
	DownCountSlider().Value(_settings.DownCount());
}

void HomePage::AutoRestoreToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	if (_settings) {
		_settings.IsAutoRestore(AutoRestoreToggleSwitch().IsOn());
	}
}

void HomePage::DownCountSlider_ValueChanged(IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
	if (_settings) {
		_settings.DownCount((uint32_t)args.NewValue());
	}
}

}
