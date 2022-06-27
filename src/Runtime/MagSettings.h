#pragma once

#include "MagSettings.g.h"

namespace winrt::Magpie::Runtime::implementation {

struct MagSettings : MagSettingsT<MagSettings> {
    MagSettings() = default;

    CaptureMode CaptureMode() const noexcept {
        return _captureMode;
    }

    void CaptureMode(Magpie::Runtime::CaptureMode value) noexcept {
        _captureMode = value;
    }

    bool IsBreakpointMode() const noexcept {
        return _isBreakpointMode;
    }

    void IsBreakpointMode(bool value) noexcept {
        _isBreakpointMode = value;
    }

private:
    Magpie::Runtime::CaptureMode _captureMode = Magpie::Runtime::CaptureMode::GraphicsCapture;
    bool _isBreakpointMode = false;
};

}

namespace winrt::Magpie::Runtime::factory_implementation {

struct MagSettings : MagSettingsT<MagSettings, implementation::MagSettings> {
};

}
