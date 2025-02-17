#pragma once
#include <parallel_hashmap/phmap.h>

namespace Magpie {

enum class CaptureMethod {
	GraphicsCapture,
	DesktopDuplication,
	GDI,
	DwmSharedSurface,
};

enum class MultiMonitorUsage {
	Closest,
	Intersected,
	All,
};

enum class CursorInterpolationMode {
	NearestNeighbor,
	Bilinear,
};

struct Cropping {
	float Left;
	float Top;
	float Right;
	float Bottom;
};

struct GraphicsCardId {
	// idx 为显卡索引，vendorId 和 deviceId 用于验证，如果不匹配则遍历显卡查找匹配。这可以处理显卡
	// 改变的情况，比如某些笔记本电脑可以在混合架构和独显直连之间切换。
	// idx 有两个作用，一是作为性能优化，二是用于区分同一型号的两个显卡。
	// idx 为 -1 表示使用默认显卡，如果此时 vendorId 和 deviceId 有值表示由于目前不存在该显卡因此
	// 使用默认显卡，如果以后该显卡再次可用将自动使用。
	int idx = -1;
	uint32_t vendorId = 0;
	uint32_t deviceId = 0;
};

struct ScalingFlags {
	static constexpr uint32_t DisableWindowResizing = 1;
	static constexpr uint32_t DebugMode = 1 << 1;
	static constexpr uint32_t DisableEffectCache = 1 << 2;
	static constexpr uint32_t SaveEffectSources = 1 << 3;
	static constexpr uint32_t WarningsAreErrors = 1 << 4;
	static constexpr uint32_t SimulateExclusiveFullscreen = 1 << 5;
	static constexpr uint32_t Is3DGameMode = 1 << 6;
	static constexpr uint32_t ShowFPS = 1 << 7;
	static constexpr uint32_t CaptureTitleBar = 1 << 10;
	static constexpr uint32_t AdjustCursorSpeed = 1 << 11;
	static constexpr uint32_t DrawCursor = 1 << 12;
	static constexpr uint32_t DisableDirectFlip = 1 << 13;
	static constexpr uint32_t DisableFontCache = 1 << 14;
	static constexpr uint32_t AllowScalingMaximized = 1 << 15;
	static constexpr uint32_t EnableStatisticsForDynamicDetection = 1 << 16;
	// Magpie.Core 不负责启动 TouchHelper.exe，指定此标志会使 Magpie.Core 创建辅助窗口以拦截
	// 黑边上的触控输入。
	static constexpr uint32_t IsTouchSupportEnabled = 1 << 17;
	static constexpr uint32_t InlineParams = 1 << 18;
	static constexpr uint32_t IsFP16Disabled = 1 << 19;
	static constexpr uint32_t BenchmarkMode = 1 << 20;
	static constexpr uint32_t DeveloperMode = 1 << 21;
	static constexpr uint32_t WindowedMode = 1 << 22;
};

enum class ScalingType {
	Normal,		// Scale 表示缩放倍数
	Fit,		// Scale 表示相对于屏幕能容纳的最大等比缩放的比例
	Absolute,	// Scale 表示目标大小（单位为像素）
	Fill		// 充满屏幕，此时不使用 Scale 参数
};

struct EffectOption {
	std::wstring name;
	phmap::flat_hash_map<std::wstring, float> parameters;
	ScalingType scalingType = ScalingType::Normal;
	std::pair<float, float> scale = { 1.0f,1.0f };

	bool HasScale() const noexcept {
		return scalingType != ScalingType::Normal ||
			std::abs(scale.first - 1.0f) > 1e-5 || std::abs(scale.second - 1.0f) > 1e-5;
	}
};

enum class DuplicateFrameDetectionMode {
	Always,
	Dynamic,
	Never
};

struct ScalingOptions {
	DEFINE_FLAG_ACCESSOR(IsWindowedMode, ScalingFlags::WindowedMode, flags)
	DEFINE_FLAG_ACCESSOR(IsDeveloperMode, ScalingFlags::DeveloperMode, flags)
	DEFINE_FLAG_ACCESSOR(IsDebugMode, ScalingFlags::DebugMode, flags)
	DEFINE_FLAG_ACCESSOR(IsBenchmarkMode, ScalingFlags::BenchmarkMode, flags)
	DEFINE_FLAG_ACCESSOR(IsFP16Disabled, ScalingFlags::IsFP16Disabled, flags)
	DEFINE_FLAG_ACCESSOR(IsEffectCacheDisabled, ScalingFlags::DisableEffectCache, flags)
	DEFINE_FLAG_ACCESSOR(IsFontCacheDisabled, ScalingFlags::DisableFontCache, flags)
	DEFINE_FLAG_ACCESSOR(IsSaveEffectSources, ScalingFlags::SaveEffectSources, flags)
	DEFINE_FLAG_ACCESSOR(IsWarningsAreErrors, ScalingFlags::WarningsAreErrors, flags)
	DEFINE_FLAG_ACCESSOR(IsStatisticsForDynamicDetectionEnabled, ScalingFlags::EnableStatisticsForDynamicDetection, flags)
	DEFINE_FLAG_ACCESSOR(IsInlineParams, ScalingFlags::InlineParams, flags)
	DEFINE_FLAG_ACCESSOR(IsTouchSupportEnabled, ScalingFlags::IsTouchSupportEnabled, flags)
	DEFINE_FLAG_ACCESSOR(IsAllowScalingMaximized, ScalingFlags::AllowScalingMaximized, flags)
	DEFINE_FLAG_ACCESSOR(IsSimulateExclusiveFullscreen, ScalingFlags::SimulateExclusiveFullscreen, flags)
	DEFINE_FLAG_ACCESSOR(Is3DGameMode, ScalingFlags::Is3DGameMode, flags)
	DEFINE_FLAG_ACCESSOR(IsShowFPS, ScalingFlags::ShowFPS, flags)
	DEFINE_FLAG_ACCESSOR(IsWindowResizingDisabled, ScalingFlags::DisableWindowResizing, flags)
	DEFINE_FLAG_ACCESSOR(IsCaptureTitleBar, ScalingFlags::CaptureTitleBar, flags)
	DEFINE_FLAG_ACCESSOR(IsAdjustCursorSpeed, ScalingFlags::AdjustCursorSpeed, flags)
	DEFINE_FLAG_ACCESSOR(IsDrawCursor, ScalingFlags::DrawCursor, flags)
	DEFINE_FLAG_ACCESSOR(IsDirectFlipDisabled, ScalingFlags::DisableDirectFlip, flags)

	Cropping cropping{};
	uint32_t flags = ScalingFlags::AdjustCursorSpeed | ScalingFlags::DrawCursor;	// ScalingFlags
	GraphicsCardId graphicsCardId;
	float minFrameRate = 0.0f;
	std::optional<float> maxFrameRate;
	float cursorScaling = 1.0f;
	CaptureMethod captureMethod = CaptureMethod::GraphicsCapture;
	MultiMonitorUsage multiMonitorUsage = MultiMonitorUsage::Closest;
	CursorInterpolationMode cursorInterpolationMode = CursorInterpolationMode::NearestNeighbor;

	std::vector<EffectOption> effects;

	DuplicateFrameDetectionMode duplicateFrameDetectionMode = DuplicateFrameDetectionMode::Dynamic;

	void ResolveConflicts() noexcept;
	void Log() const noexcept;
};

}
