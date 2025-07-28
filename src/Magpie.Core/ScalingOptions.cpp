#include "pch.h"
#include "ScalingOptions.h"
#include "Logger.h"
#include "StrHelper.h"

namespace Magpie {

static std::string LogParameters(const phmap::flat_hash_map<std::string, float>& params) noexcept {
	std::string result;

	if (params.empty()) {
		result = "无";
	} else {
		for (const auto& pair : params) {
			result.append(fmt::format("\n\t\t\t\t{}: {}", pair.first, pair.second));
		}
	}
	
	return result;
}

static std::string LogEffects(const std::vector<EffectOption>& effects) noexcept {
	std::string result;
	for (const EffectOption& effect : effects) {
		result.append(fmt::format(R"(
		{}
			scalingType: {}
			scale: {},{}
			parameters: {})",
			effect.name,
			(int)effect.scalingType,
			effect.scale.first, effect.scale.second,
			LogParameters(effect.parameters)
		));
	}
	return result;
}

void ScalingOptions::Log() const noexcept {
	Logger::Get().Info(fmt::format(R"(缩放选项
	IsWindowedMode: {}
	IsDebugMode: {}
	IsBenchmarkMode: {}
	IsFP16Disabled: {}
	IsEffectCacheDisabled: {}
	IsFontCacheDisabled: {}
	IsSaveEffectSources: {}
	IsWarningsAreErrors: {}
	IsStatisticsForDynamicDetectionEnabled: {}
	IsInlineParams: {}
	IsTouchSupportEnabled: {}
	IsAllowScalingMaximized: {}
	IsSimulateExclusiveFullscreen: {}
	Is3DGameMode: {}
	IsCaptureTitleBar: {}
	IsAdjustCursorSpeed: {}
	IsDirectFlipDisabled: {}
	cropping: {},{},{},{}
	graphicsCardId:
		idx: {}
		venderId: {}
		deviceId: {}
	minFrameRate: {}
	maxFrameRate: {}
	cursorScaling: {}
	captureMethod: {}
	multiMonitorUsage: {}
	cursorInterpolationMode: {}
	duplicateFrameDetectionMode: {}
	fullscreenInitialToolbarState: {}
	windowedInitialToolbarState: {}
	screenshotsDir: {}
	effects: {})",
		IsWindowedMode(),
		IsDebugMode(),
		IsBenchmarkMode(),
		IsFP16Disabled(),
		IsEffectCacheDisabled(),
		IsFontCacheDisabled(),
		IsSaveEffectSources(),
		IsWarningsAreErrors(),
		IsStatisticsForDynamicDetectionEnabled(),
		IsInlineParams(),
		IsTouchSupportEnabled(),
		IsAllowScalingMaximized(),
		IsSimulateExclusiveFullscreen(),
		Is3DGameMode(),
		IsCaptureTitleBar(),
		IsAdjustCursorSpeed(),
		IsDirectFlipDisabled(),
		cropping.Left, cropping.Top, cropping.Right, cropping.Bottom,
		graphicsCardId.idx,
		graphicsCardId.vendorId,
		graphicsCardId.deviceId,
		minFrameRate,
		maxFrameRate.has_value() ? *maxFrameRate : 0.0f,
		cursorScaling,
		(int)captureMethod,
		(int)multiMonitorUsage,
		(int)cursorInterpolationMode,
		(int)duplicateFrameDetectionMode,
		(int)fullscreenInitialToolbarState,
		(int)windowedInitialToolbarState,
		StrHelper::UTF16ToUTF8(screenshotsDir.native()),
		LogEffects(effects)
	));
}

}
