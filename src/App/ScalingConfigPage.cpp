#include "pch.h"
#include "ScalingConfigPage.h"
#if __has_include("ScalingConfigPage.g.cpp")
#include "ScalingConfigPage.g.cpp"
#endif
#include "Win32Utils.h"
#include "ComboBoxHelper.h"

using namespace winrt;


namespace winrt::Magpie::App::implementation {

ScalingConfigPage::ScalingConfigPage() {
    InitializeComponent();

    App app = Application::Current().as<App>();
    _magSettings = app.Settings().GetMagSettings(0);

    if (Win32Utils::GetOSBuild() < 22000) {
        // Segoe MDL2 Assets 不存在 Move 图标
        AdjustCursorSpeedFontIcon().Glyph(L"\uE962");
    }

    CaptureModeComboBox().SelectedIndex((int32_t)_magSettings.CaptureMode());
    Is3DGameModeToggleSwitch().IsOn(_magSettings.Is3DGameMode());
}

void ScalingConfigPage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
    ComboBoxHelper::DropDownOpened(*this, sender);
}

void ScalingConfigPage::CaptureModeComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&) {
    if (_magSettings) {
        _magSettings.CaptureMode((Magpie::Runtime::CaptureMode)CaptureModeComboBox().SelectedIndex());
    }
}

void ScalingConfigPage::Is3DGameModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
    _magSettings.Is3DGameMode(Is3DGameModeToggleSwitch().IsOn());
}

}
