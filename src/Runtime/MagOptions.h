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

#define DEFINE_MAGFLAG_ACCESSOR(Name) \
	bool Name() const noexcept { return Flags & ::Magpie::Runtime::MagFlags::Name; } \
	void Name(bool value) noexcept { \
		if (value) { \
			Flags |= ::Magpie::Runtime::MagFlags::Name; \
		} else { \
			Flags &= ~::Magpie::Runtime::MagFlags::Name; \
		} \
	}

struct MagOptions {
	DEFINE_MAGFLAG_ACCESSOR(IsDisableWindowResizing)
	DEFINE_MAGFLAG_ACCESSOR(IsBreakpointMode)
	DEFINE_MAGFLAG_ACCESSOR(IsDisableEffectCache)
	DEFINE_MAGFLAG_ACCESSOR(IsSaveEffectSources)
	DEFINE_MAGFLAG_ACCESSOR(IsWarningsAreErrors)
	DEFINE_MAGFLAG_ACCESSOR(IsSimulateExclusiveFullscreen)
	DEFINE_MAGFLAG_ACCESSOR(Is3DGameMode)
	DEFINE_MAGFLAG_ACCESSOR(IsShowFPS)
	DEFINE_MAGFLAG_ACCESSOR(IsVSync)
	DEFINE_MAGFLAG_ACCESSOR(IsTripleBuffering)
	DEFINE_MAGFLAG_ACCESSOR(IsReserveTitleBar)
	DEFINE_MAGFLAG_ACCESSOR(IsAdjustCursorSpeed)
	DEFINE_MAGFLAG_ACCESSOR(IsDrawCursor)
	DEFINE_MAGFLAG_ACCESSOR(IsDisableDirectFlip)

	Cropping Cropping{};
	uint32_t Flags = MagFlags::IsVSync | MagFlags::IsAdjustCursorSpeed | MagFlags::IsDrawCursor;
	uint32_t GraphicsAdapter = 0;
	float CursorScaling = 1.0f;
	CaptureMode CaptureMode = CaptureMode::GraphicsCapture;
	MultiMonitorUsage MultiMonitorUsage = MultiMonitorUsage::Nearest;
	CursorInterpolationMode CursorInterpolationMode = CursorInterpolationMode::Nearest;
};

}
