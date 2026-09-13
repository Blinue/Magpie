#pragma once
#include <parallel_hashmap/phmap.h>

namespace Magpie {

enum class CaptureMethod {
	GraphicsCapture,
	DesktopDuplication,
	GDI,
	DwmSharedSurface,
	COUNT
};

enum class MultiMonitorUsage {
	Closest,
	Intersected,
	All,
	COUNT
};

enum class CursorInterpolationMode {
	NearestNeighbor,
	Bilinear,
	COUNT
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

enum class OutputAlignment {
	LeftTop,
	Top,
	RightTop,
	Left,
	Center,
	Right,
	LeftBottom,
	Bottom,
	RightBottom,
	COUNT
};

enum class ScalingType {
	Normal,		// Scale 表示缩放倍数
	Fit,		// Scale 表示相对于屏幕能容纳的最大等比缩放的比例
	Absolute,	// Scale 表示目标大小（单位为像素）
	Fill		// 充满屏幕，此时不使用 Scale 参数
};

struct EffectOption {
	std::string name;
	phmap::flat_hash_map<std::string, float> parameters;
	ScalingType scalingType = ScalingType::Normal;
	std::pair<float, float> scale = { 1.0f,1.0f };

	bool HasScale() const noexcept {
		return scalingType != ScalingType::Normal ||
			!IsApprox(scale.first, 1.0f) || !IsApprox(scale.second, 1.0f);
	}
};

enum class DuplicateFrameDetectionMode {
	Always,
	Dynamic,
	Never,
	COUNT
};

enum class ToolbarState {
	Off,
	AlwaysShow,
	AutoHide,
	COUNT
};

struct OverlayWindowOption {
	// 0: 位于左侧，hPos 是窗口左边界和画面左边界距离（所有距离都是应用 DPI 缩放前的值）
	// 1: 位于中侧，hPos 是窗口中心点和画面左边界距离与画面宽度之比
	// 2: 位于右侧，hPos 是窗口右边界和画面右边界距离
	uint16_t hArea = 0;
	// 0: 位于上侧，vPos 是窗口上边界和画面上边界距离
	// 1: 位于中侧，vPos 是窗口中心点和画面上边界距离与画面高度之比
	// 2: 位于下侧，vPos 是窗口下边界和画面下边界距离
	uint16_t vArea = 0;
	float hPos = 0.0f;
	float vPos = 0.0f;
};

struct OverlayOptions {
	phmap::flat_hash_map<std::string, OverlayWindowOption> windows;
	std::string scaleShortcut;
	std::string windowedModeScaleShortcut;
	std::string takeScreenshotShortcut;
};

enum class ScalingError {
	NoError,

	/////////////////////////////////////
	// 
	// 先决条件错误
	// 
	/////////////////////////////////////

	// 未配置缩放模式或者缩放模式不合法
	InvalidScalingMode,
	// 启用触控支持失败
	TouchSupport,
	// 3D 游戏模式下不支持窗口模式缩放
	Windowed3DGameMode,
	// Desktop Duplication 不支持窗口模式缩放
	WindowedDesktopDuplication,
	// 通用的不支持缩放错误
	InvalidSourceWindow,
	// 因窗口已最大化或全屏而无法缩放，可通过更改设置强制缩放
	Maximized,
	// 因窗口的 IL 更高而无法缩放
	LowIntegrityLevel,
	// 应用自定义裁剪后尺寸太小或为负
	InvalidCropping,
	// 窗口不符合窗口模式缩放的条件，如已最大化
	BannedInWindowedMode,

	/////////////////////////////////////
	//
	// 初始化和缩放时错误
	//
	/////////////////////////////////////

	// 通用的缩放失败错误
	ScalingFailedGeneral,
	// FrameSource 初始化失败
	CaptureFailed,
	// ID3D11Device5::CreateFence 失败
	CreateFenceFailed
};

enum class ScalingFlags : uint32_t {
	None,
	WindowedMode = 1,
	DebugMode = 1 << 1,
	DisableEffectCache = 1 << 2,
	SaveEffectSources = 1 << 3,
	WarningsAreErrors = 1 << 4,
	KeepScreenOn = 1 << 5,
	SimulateExclusiveFullscreen = 1 << 6,
	Is3DGameMode = 1 << 7,
	CaptureTitleBar = 1 << 8,
	AdjustCursorSpeed = 1 << 9,
	DisableDirectFlip = 1 << 10,
	DisableFontCache = 1 << 11,
	AllowScalingMaximized = 1 << 12,
	EnableStatisticsForDynamicDetection = 1 << 13,
	// 只影响缩放行为，Magpie.Core 不负责启动 TouchHelper.exe
	TouchSupportEnabled = 1 << 14,
	InlineParams = 1 << 15,
	DisableFP16 = 1 << 16,
	BenchmarkMode = 1 << 17,
	DeveloperMode = 1 << 18,
	DisableTopmost = 1 << 19
};
DEFINE_ENUM_FLAG_OPERATORS(ScalingFlags)

struct ScalingOptions {
	DEFINE_FLAG_ACCESSOR(IsWindowedMode, ScalingFlags::WindowedMode, flags)
	DEFINE_FLAG_ACCESSOR(IsDebugMode, ScalingFlags::DebugMode, flags)
	DEFINE_FLAG_ACCESSOR(IsEffectCacheDisabled, ScalingFlags::DisableEffectCache, flags)
	DEFINE_FLAG_ACCESSOR(IsSaveEffectSources, ScalingFlags::SaveEffectSources, flags)
	DEFINE_FLAG_ACCESSOR(IsWarningsAreErrors, ScalingFlags::WarningsAreErrors, flags)
	DEFINE_FLAG_ACCESSOR(IsKeepScreenOn, ScalingFlags::KeepScreenOn, flags)
	DEFINE_FLAG_ACCESSOR(IsSimulateExclusiveFullscreen, ScalingFlags::SimulateExclusiveFullscreen, flags)
	DEFINE_FLAG_ACCESSOR(Is3DGameMode, ScalingFlags::Is3DGameMode, flags)
	DEFINE_FLAG_ACCESSOR(IsCaptureTitleBar, ScalingFlags::CaptureTitleBar, flags)
	DEFINE_FLAG_ACCESSOR(IsAdjustCursorSpeed, ScalingFlags::AdjustCursorSpeed, flags)
	DEFINE_FLAG_ACCESSOR(IsDirectFlipDisabled, ScalingFlags::DisableDirectFlip, flags)
	DEFINE_FLAG_ACCESSOR(IsFontCacheDisabled, ScalingFlags::DisableFontCache, flags)
	DEFINE_FLAG_ACCESSOR(IsAllowScalingMaximized, ScalingFlags::AllowScalingMaximized, flags)
	DEFINE_FLAG_ACCESSOR(IsStatisticsForDynamicDetectionEnabled, ScalingFlags::EnableStatisticsForDynamicDetection, flags)
	DEFINE_FLAG_ACCESSOR(IsTouchSupportEnabled, ScalingFlags::TouchSupportEnabled, flags)
	DEFINE_FLAG_ACCESSOR(IsInlineParams, ScalingFlags::InlineParams, flags)
	DEFINE_FLAG_ACCESSOR(IsFP16Disabled, ScalingFlags::DisableFP16, flags)
	DEFINE_FLAG_ACCESSOR(IsBenchmarkMode, ScalingFlags::BenchmarkMode, flags)
	DEFINE_FLAG_ACCESSOR(IsDeveloperMode, ScalingFlags::DeveloperMode, flags)
	DEFINE_FLAG_ACCESSOR(IsTopmostDisabled, ScalingFlags::DisableTopmost, flags)

	std::vector<EffectOption> effects;
	ScalingFlags flags = ScalingFlags::AdjustCursorSpeed;
	Cropping cropping{};
	GraphicsCardId graphicsCardId;
	float minFrameRate = 0.0f;
	std::optional<float> maxFrameRate;
	float cursorScaleFactor = 1.0f;
	CaptureMethod captureMethod = CaptureMethod::GraphicsCapture;
	MultiMonitorUsage multiMonitorUsage = MultiMonitorUsage::Closest;
	OutputAlignment outputAlignment = OutputAlignment::Center;
	CursorInterpolationMode cursorInterpolationMode = CursorInterpolationMode::NearestNeighbor;
	std::optional<float> autoHideCursorDelay;
	DuplicateFrameDetectionMode duplicateFrameDetectionMode = DuplicateFrameDetectionMode::Dynamic;
	ToolbarState fullscreenInitialToolbarState = ToolbarState::AutoHide;
	ToolbarState windowedInitialToolbarState = ToolbarState::AutoHide;
	float initialWindowedScaleFactor = 0.0f;
	std::filesystem::path screenshotsDir;

	// 下面的成员支持在缩放时修改
	OverlayOptions overlayOptions;

	void (*showToast)(HWND hwndTarget, std::wstring_view msg) noexcept = nullptr;
	void (*showError)(HWND hwndTarget, ScalingError error) noexcept = nullptr;
	void (*save)(const ScalingOptions& options, HWND hwndScaling) noexcept = nullptr;

	void Prepare() noexcept;
};

}
