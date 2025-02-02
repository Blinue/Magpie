#pragma once
#include "ScalingOptions.h"

namespace Magpie {

enum class CursorScaling {
	x0_5,
	x0_75,
	NoScaling,
	x1_25,
	x1_5,
	x2,
	Source,
	Custom
};

// 默认规则 Name、PathRule、ClassNameRule 均为空
struct Profile {
	void Copy(const Profile& other) noexcept {
		scalingMode = other.scalingMode;
		cursorScaling = other.cursorScaling;
		customCursorScaling = other.customCursorScaling;
		cropping = other.cropping;
		captureMethod = other.captureMethod;
		graphicsCardId = other.graphicsCardId;
		maxFrameRate = other.maxFrameRate;
		multiMonitorUsage = other.multiMonitorUsage;
		cursorInterpolationMode = other.cursorInterpolationMode;
		launchParameters = other.launchParameters;
		scalingFlags = other.scalingFlags;

		isCroppingEnabled = other.isCroppingEnabled;
		isAutoScale = other.isAutoScale;
		isFrameRateLimiterEnabled = other.isFrameRateLimiterEnabled;
	}

	DEFINE_FLAG_ACCESSOR(IsWindowResizingDisabled, ScalingFlags::DisableWindowResizing, scalingFlags)
	DEFINE_FLAG_ACCESSOR(Is3DGameMode, ScalingFlags::Is3DGameMode, scalingFlags)
	DEFINE_FLAG_ACCESSOR(IsShowFPS, ScalingFlags::ShowFPS, scalingFlags)
	DEFINE_FLAG_ACCESSOR(IsCaptureTitleBar, ScalingFlags::CaptureTitleBar, scalingFlags)
	DEFINE_FLAG_ACCESSOR(IsAdjustCursorSpeed, ScalingFlags::AdjustCursorSpeed, scalingFlags)
	DEFINE_FLAG_ACCESSOR(IsDrawCursor, ScalingFlags::DrawCursor, scalingFlags)
	DEFINE_FLAG_ACCESSOR(IsDirectFlipDisabled, ScalingFlags::DisableDirectFlip, scalingFlags)

	std::wstring name;

	// 若为打包应用，PathRule 存储 AUMID
	std::wstring pathRule;
	std::wstring classNameRule;

	// 如果在同一个驱动器上则存储相对路径，否则存储绝对路径
	// 若为空使用 pathRule
	std::wstring launcherPath;

	CursorScaling cursorScaling = CursorScaling::NoScaling;
	float customCursorScaling = 1.0;

	Cropping cropping{};
	// -1 表示原样
	int scalingMode = -1;
	CaptureMethod captureMethod = CaptureMethod::GraphicsCapture;
	GraphicsCardId graphicsCardId;
	MultiMonitorUsage multiMonitorUsage = MultiMonitorUsage::Closest;
	CursorInterpolationMode cursorInterpolationMode = CursorInterpolationMode::NearestNeighbor;

	// 10~1000
	float maxFrameRate = 60.0f;

	std::wstring launchParameters;

	uint32_t scalingFlags = ScalingFlags::AdjustCursorSpeed | ScalingFlags::DrawCursor;

	bool isPackaged = false;
	bool isCroppingEnabled = false;
	bool isAutoScale = false;
	bool isFrameRateLimiterEnabled = false;
};

}
