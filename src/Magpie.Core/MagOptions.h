#pragma once
#include "pch.h"
#include <parallel_hashmap/phmap.h>


namespace Magpie::Core {

enum class CaptureMethod {
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
	static constexpr const uint32_t DisableWindowResizing = 0x1;
	static constexpr const uint32_t BreakpointMode = 0x2;
	static constexpr const uint32_t DisableEffectCache = 0x4;
	static constexpr const uint32_t SaveEffectSources = 0x8;
	static constexpr const uint32_t WarningsAreErrors = 0x10;
	static constexpr const uint32_t SimulateExclusiveFullscreen = 0x20;
	static constexpr const uint32_t Is3DGameMode = 0x40;
	static constexpr const uint32_t ShowFPS = 0x80;
	static constexpr const uint32_t VSync = 0x100;
	static constexpr const uint32_t TripleBuffering = 0x200;
	static constexpr const uint32_t ReserveTitleBar = 0x400;
	static constexpr const uint32_t AdjustCursorSpeed = 0x800;
	static constexpr const uint32_t DrawCursor = 0x1000;
	static constexpr const uint32_t DisableDirectFlip = 0x2000;
};

struct DownscalingEffect {
	std::wstring name;
	phmap::flat_hash_map<std::wstring, float> parameters;
};

enum class ScalingType {
	Normal,		// Scale 表示缩放倍数
	Fit,		// Scale 表示相对于屏幕能容纳的最大等比缩放的比例
	Absolute,	// Scale 表示目标大小（单位为像素）
	Fill		// 充满屏幕，此时不使用 Scale 参数
};

struct EffectOptionFlags {
	static constexpr const uint32_t InlineParams = 0x1;
	static constexpr const uint32_t FP16 = 0x2;
};

struct EffectOption {
	std::wstring name;
	phmap::flat_hash_map<std::wstring, float> parameters;
	ScalingType scalingType = ScalingType::Normal;
	std::pair<float, float> scale = { 1.0f,1.0f };
	uint32_t flags = 0;	// EffectOptionFlags

	bool HasScale() const noexcept {
		return scalingType != ScalingType::Normal ||
			std::abs(scale.first - 1.0f) > 1e-5 || std::abs(scale.second - 1.0f) > 1e-5;
	}
};

struct MagOptions {
	DEFINE_FLAG_ACCESSOR(IsDisableWindowResizing, MagFlags::DisableWindowResizing, flags)
	DEFINE_FLAG_ACCESSOR(IsDebugMode, MagFlags::BreakpointMode, flags)
	DEFINE_FLAG_ACCESSOR(IsDisableEffectCache, MagFlags::DisableEffectCache, flags)
	DEFINE_FLAG_ACCESSOR(IsSaveEffectSources, MagFlags::SaveEffectSources, flags)
	DEFINE_FLAG_ACCESSOR(IsWarningsAreErrors, MagFlags::WarningsAreErrors, flags)
	DEFINE_FLAG_ACCESSOR(IsSimulateExclusiveFullscreen, MagFlags::SimulateExclusiveFullscreen, flags)
	DEFINE_FLAG_ACCESSOR(Is3DGameMode, MagFlags::Is3DGameMode, flags)
	DEFINE_FLAG_ACCESSOR(IsShowFPS, MagFlags::ShowFPS, flags)
	DEFINE_FLAG_ACCESSOR(IsVSync, MagFlags::VSync, flags)
	DEFINE_FLAG_ACCESSOR(IsTripleBuffering, MagFlags::TripleBuffering, flags)
	DEFINE_FLAG_ACCESSOR(IsReserveTitleBar, MagFlags::ReserveTitleBar, flags)
	DEFINE_FLAG_ACCESSOR(IsAdjustCursorSpeed, MagFlags::AdjustCursorSpeed, flags)
	DEFINE_FLAG_ACCESSOR(IsDrawCursor, MagFlags::DrawCursor, flags)
	DEFINE_FLAG_ACCESSOR(IsDisableDirectFlip, MagFlags::DisableDirectFlip, flags)

	Cropping cropping{};
	uint32_t flags = MagFlags::VSync | MagFlags::AdjustCursorSpeed | MagFlags::DrawCursor;	// MagFlags
	uint32_t graphicsAdapter = 0;
	float cursorScaling = 1.0f;
	CaptureMethod captureMethod = CaptureMethod::GraphicsCapture;
	MultiMonitorUsage multiMonitorUsage = MultiMonitorUsage::Nearest;
	CursorInterpolationMode cursorInterpolationMode = CursorInterpolationMode::Nearest;

	DownscalingEffect downscalingEffect;

	std::vector<EffectOption> effects;
};

}
