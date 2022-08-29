#pragma once
#include "pch.h"


namespace Magpie::Core {

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

enum class ScaleType {
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
	std::unordered_map<std::wstring, float> parameters;
	ScaleType scaleType = ScaleType::Normal;
	std::pair<float, float> scale = { 1.0f,1.0f };
	uint32_t flags = 0;	// EffectOptionFlags

	bool HasScale() const noexcept {
		return scaleType != ScaleType::Normal ||
			std::abs(scale.first - 1.0f) > 1e-5 || std::abs(scale.second - 1.0f) > 1e-5;
	}
};

#define DEFINE_MAGFLAG_ACCESSOR(Name) \
	bool Name() const noexcept { return flags & ::Magpie::Core::MagFlags::Name; } \
	void Name(bool value) noexcept { \
		if (value) { \
			flags |= ::Magpie::Core::MagFlags::Name; \
		} else { \
			flags &= ~::Magpie::Core::MagFlags::Name; \
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

	Cropping cropping{};
	uint32_t flags = MagFlags::IsVSync | MagFlags::IsAdjustCursorSpeed | MagFlags::IsDrawCursor;	// MagFlags
	uint32_t graphicsAdapter = 0;
	float cursorScaling = 1.0f;
	CaptureMode captureMode = CaptureMode::GraphicsCapture;
	MultiMonitorUsage multiMonitorUsage = MultiMonitorUsage::Nearest;
	CursorInterpolationMode cursorInterpolationMode = CursorInterpolationMode::Nearest;

	std::vector<EffectOption> effects;
};

}
