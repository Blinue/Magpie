#include "pch.h"
#include "ScalingConfigPage.h"
#if __has_include("ScalingConfigPage.g.cpp")
#include "ScalingConfigPage.g.cpp"
#endif

using namespace winrt;


namespace winrt::Magpie::App::implementation {

ScalingConfigPage::ScalingConfigPage() {
    InitializeComponent();

    App app = Application::Current().as<App>();
    _magSettings = app.Settings().GetMagSettings(0);

    CaptureModeComboBox().SelectedIndex((int32_t)_magSettings.CaptureMode());
}

void ScalingConfigPage::CaptureModeComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&) {
    if (_magSettings) {
        _magSettings.CaptureMode((Magpie::Runtime::CaptureMode)CaptureModeComboBox().SelectedIndex());
    }
}

}
