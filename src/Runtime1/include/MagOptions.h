#pragma once
#include "CommonPCH.h"


namespace Magpie::Runtime {

enum class CaptureMode {
	GraphicsCapture,
	DesktopDuplication,
	GDI,
	DwmSharedSurface,
};

enum class MultiMonitorUsage {
	Nearest,
	Intersected,
	All,
};

enum class CursorInterpolationMode {
	Nearest,
	Bilinear,
};

struct Cropping {
	float Left;
	float Top;
	float Right;
	float Bottom;
};

struct MagFlags {
	static constexpr const uint32_t IsDisableWindowResizing = 0x1;
	static constexpr const uint32_t IsBreakpointMode = 0x2;
	static constexpr const uint32_t IsDisableEffectCache = 0x4;
	static constexpr const uint32_t IsSaveEffectSources = 0x8;
	static constexpr const uint32_t IsWarningsAreErrors = 0x10;
	static constexpr const uint32_t IsSimulateExclusiveFullscreen = 0x20;
	static constexpr const uint32_t Is3DGameMode = 0x40;
	static constexpr const uint32_t IsShowFPS = 0x80;
	static constexpr const uint32_t IsVSync = 0x100;
	static constexpr const uint32_t IsTripleBuffering = 0x200;
	static constexpr const uint32_t IsReserveTitleBar = 0x400;
	static constexpr const uint32_t IsAdjustCursorSpeed = 0x800;
	static constexpr const uint32_t IsDrawCursor = 0x1000;
	static constexpr const uint32_t IsDisableDirectFlip = 0x2000;
};

struct MagOptions {
	Cropping Cropping{};
	uint32_t GraphicsAdapter = 0;
	float CursorScaling = 1.0f;

	uint32_t Flags = 0;

	CaptureMode CaptureMode = CaptureMode::GraphicsCapture;
	MultiMonitorUsage MultiMonitorUsage = MultiMonitorUsage::Nearest;
	CursorInterpolationMode CrsorInterpolationMode = CursorInterpolationMode::Nearest;
};

}
