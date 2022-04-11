#pragma once
#include "pch.h"


class Config {
public:
	Config() = default;
	~Config() = default;

	Config(const Config&) = delete;
	Config(Config&&) = delete;

	bool Initialize(
		float cursorZoomFactor,
		UINT cursorInterpolationMode,
		int adapterIdx,
		UINT multiMonitorUsage,
		const RECT& cropBorders,
		UINT flags
	);

	float GetCursorZoomFactor() const noexcept {
		return _cursorZoomFactor;
	}

	UINT GetCursorInterpolationMode() const noexcept {
		return _cursorInterpolationMode;
	}

	int GetAdapterIdx() const noexcept {
		return _adapterIdx;
	}

	UINT GetMultiMonitorUsage() const noexcept {
		return _multiMonitorUsage;
	}

	const RECT& GetCropBorders() const noexcept {
		return _cropBorders;
	}

	bool IsNoCursor() const noexcept {
		return _isNoCursor;
	}

	bool IsAdjustCursorSpeed() const noexcept {
		return _isAdjustCursorSpeed;
	}

	bool IsDisableLowLatency() const noexcept {
		return _isDisableLowLatency;
	}

	bool IsDisableWindowResizing() const noexcept {
		return _isDisableWindowResizing;
	}

	bool IsBreakpointMode() const noexcept {
		return _isBreakpointMode;
	}

	bool IsDisableDirectFlip() const noexcept {
		return _isDisableDirectFlip;
	}

	bool IsConfineCursorIn3DGames() const noexcept {
		return _isConfineCursorIn3DGames;
	}

	bool IsCropTitleBarOfUWP() const noexcept {
		return _isCropTitleBarOfUWP;
	}

	bool IsDisableEffectCache() const noexcept {
		return _isDisableEffectCache;
	}

	bool IsSimulateExclusiveFullscreen() const noexcept {
		return _isSimulateExclusiveFullscreen;
	}

	bool IsDisableVSync() const noexcept {
		return _isDisableVSync;
	}

	bool IsSaveEffectSources() const noexcept {
		return _isSaveEffectSources;
	}

	bool IsTreatWarningsAsErrors() const noexcept {
		return _isTreatWarningsAsErrors;
	}

private:
	RECT _cropBorders{};
	UINT _multiMonitorUsage = 0;
	int _adapterIdx = 0;

	float _cursorZoomFactor = 1.0f;
	UINT _cursorInterpolationMode = 0;

	bool _isNoCursor = false;
	bool _isAdjustCursorSpeed = false;
	bool _isDisableLowLatency = false;
	bool _isDisableWindowResizing = false;
	bool _isDisableDirectFlip = false;
	bool _isConfineCursorIn3DGames = false;
	bool _isCropTitleBarOfUWP = false;
	bool _isSimulateExclusiveFullscreen = false;
	bool _isDisableVSync = false;

	// 用于调试
	bool _isBreakpointMode = false;
	bool _isDisableEffectCache = false;
	bool _isSaveEffectSources = false;
	bool _isTreatWarningsAsErrors = false;
};

