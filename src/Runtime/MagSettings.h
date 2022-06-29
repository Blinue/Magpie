#pragma once

#include "MagSettings.g.h"

namespace winrt::Magpie::Runtime::implementation {

struct MagSettings : MagSettingsT<MagSettings> {
    MagSettings() = default;

    void CopyFrom(Magpie::Runtime::MagSettings other);

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

    bool IsDisableEffectCache() const noexcept {
        return _isDisableEffectCache;
    }

    void IsDisableEffectCache(bool value) noexcept {
        _isDisableEffectCache = value;
    }

    bool IsSaveEffectSources() const noexcept {
        return _isSaveEffectSources;
    }

    void IsSaveEffectSources(bool value) noexcept {
        _isSaveEffectSources = value;
    }

    bool IsWarningsAreErrors() const noexcept {
        return _isWarningsAreErrors;
    }

    void IsWarningsAreErrors(bool value) noexcept {
        _isWarningsAreErrors = value;
    }

    bool IsSimulateExclusiveFullscreen() const noexcept {
        return _isSimulateExclusiveFullscreen;
    }

    void IsSimulateExclusiveFullscreen(bool value) noexcept {
        _isSimulateExclusiveFullscreen = value;
    }

    bool Is3DGameMode() const noexcept {
        return _is3DGameMode;
    }

    void Is3DGameMode(bool value) noexcept {
        _is3DGameMode = value;
    }

    bool IsShowFPS() const noexcept {
        return _isShowFPS;
    }

    void IsShowFPS(bool value) noexcept {
        _isShowFPS = value;
    }

private:
    Magpie::Runtime::CaptureMode _captureMode = Magpie::Runtime::CaptureMode::GraphicsCapture;

    bool _isBreakpointMode = false;
    bool _isDisableEffectCache = false;
    bool _isSaveEffectSources = false;
    bool _isWarningsAreErrors = false;
    bool _isSimulateExclusiveFullscreen = false;
    bool _is3DGameMode = false;
    bool _isShowFPS = false;
};

}

namespace winrt::Magpie::Runtime::factory_implementation {

struct MagSettings : MagSettingsT<MagSettings, implementation::MagSettings> {
};

}
