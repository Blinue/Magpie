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

	MultiMonitorUsage MultiMonitorUsage() const noexcept {
		return _multiMonitorUsage;
	}

	void MultiMonitorUsage(Magpie::Runtime::MultiMonitorUsage value) noexcept {
		_multiMonitorUsage = value;
	}

	uint32_t GraphicsAdapter() const noexcept {
		return _graphicsAdapter;
	}

	void GraphicsAdapter(uint32_t value) {
		_graphicsAdapter = value;
	}

	bool IsDisableWindowResizing() const noexcept {
		return _isDisableWindowResizing;
	}

	void IsDisableWindowResizing(bool value) noexcept {
		_isDisableWindowResizing = value;
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

	bool IsVSync() const noexcept {
		return _isVSync;
	}

	void IsVSync(bool value) noexcept {
		_isVSync = value;
	}

	bool IsTripleBuffering() const noexcept {
		return _isTripleBuffering;
	}

	void IsTripleBuffering(bool value) noexcept {
		_isTripleBuffering = value;
	}

	bool IsReserveTitleBar() const noexcept {
		return _isReserveTitleBar;
	}

	void IsReserveTitleBar(bool value) noexcept {
		_isReserveTitleBar = value;
	}

	Cropping Cropping() noexcept {
		return _cropping;
	}

	void Cropping(const Magpie::Runtime::Cropping& value) noexcept {
		_cropping = value;
	}

	bool IsAdjustCursorSpeed() const noexcept {
		return _isAdjustCursorSpeed;
	}

	void IsAdjustCursorSpeed(bool value) noexcept {
		_isAdjustCursorSpeed = value;
	}

	bool IsDrawCursor() const noexcept {
		return _isDrawCursor;
	}

	void IsDrawCursor(bool value) noexcept {
		_isDrawCursor = value;
	}

	double CursorScaling() const noexcept {
		return _cursorScaling;
	}

	void CursorScaling(double value) noexcept {
		_cursorScaling = value;
	}

private:
	Magpie::Runtime::CaptureMode _captureMode = Magpie::Runtime::CaptureMode::GraphicsCapture;
	Magpie::Runtime::MultiMonitorUsage _multiMonitorUsage = Magpie::Runtime::MultiMonitorUsage::Nearest;
	uint32_t _graphicsAdapter = 0;
	bool _isDisableWindowResizing = false;
	Magpie::Runtime::Cropping _cropping{};

	bool _isBreakpointMode = false;
	bool _isDisableEffectCache = false;
	bool _isSaveEffectSources = false;
	bool _isWarningsAreErrors = false;
	bool _isSimulateExclusiveFullscreen = false;
	bool _is3DGameMode = false;
	bool _isShowFPS = false;
	bool _isVSync = true;
	bool _isTripleBuffering = false;
	bool _isReserveTitleBar = false;
	bool _isAdjustCursorSpeed = true;
	bool _isDrawCursor = true;
	double _cursorScaling = 1.0;
};

}

namespace winrt::Magpie::Runtime::factory_implementation {

struct MagSettings : MagSettingsT<MagSettings, implementation::MagSettings> {
};

}
