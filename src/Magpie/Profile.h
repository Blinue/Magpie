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
		// 不复制自动缩放选项
		scalingMode = other.scalingMode;
		cursorScaling = other.cursorScaling;
		customCursorScaling = other.customCursorScaling;
		isCroppingEnabled = other.isCroppingEnabled;
		cropping = other.cropping;
		captureMethod = other.captureMethod;
		graphicsCard = other.graphicsCard;
		isFrameRateLimiterEnabled = other.isFrameRateLimiterEnabled;
		maxFrameRate = other.maxFrameRate;
		multiMonitorUsage = other.multiMonitorUsage;
		cursorInterpolationMode = other.cursorInterpolationMode;
		launchParameters = other.launchParameters;
		scalingFlags = other.scalingFlags;
	}

	DEFINE_FLAG_ACCESSOR(IsWindowResizingDisabled, ::Magpie::ScalingFlags::DisableWindowResizing, scalingFlags)
	DEFINE_FLAG_ACCESSOR(Is3DGameMode, ::Magpie::ScalingFlags::Is3DGameMode, scalingFlags)
	DEFINE_FLAG_ACCESSOR(IsShowFPS, ::Magpie::ScalingFlags::ShowFPS, scalingFlags)
	DEFINE_FLAG_ACCESSOR(IsCaptureTitleBar, ::Magpie::ScalingFlags::CaptureTitleBar, scalingFlags)
	DEFINE_FLAG_ACCESSOR(IsAdjustCursorSpeed, ::Magpie::ScalingFlags::AdjustCursorSpeed, scalingFlags)
	DEFINE_FLAG_ACCESSOR(IsDrawCursor, ::Magpie::ScalingFlags::DrawCursor, scalingFlags)
	DEFINE_FLAG_ACCESSOR(IsDirectFlipDisabled, ::Magpie::ScalingFlags::DisableDirectFlip, scalingFlags)

	std::wstring name;

	// 若为打包应用，PathRule 存储 AUMID
	std::wstring pathRule;
	std::wstring classNameRule;

	// 如果在同一个驱动器上则存储相对路径，否则存储绝对路径
	// 若为空使用 pathRule
	std::wstring launcherPath;

	CursorScaling cursorScaling = CursorScaling::NoScaling;
	float customCursorScaling = 1.0;

	::Magpie::Cropping cropping{};
	// -1 表示原样
	int scalingMode = -1;
	::Magpie::CaptureMethod captureMethod = ::Magpie::CaptureMethod::GraphicsCapture;
	// -1 表示默认，大于等于 0 为图形适配器的索引
	int graphicsCard = -1;
	::Magpie::MultiMonitorUsage multiMonitorUsage = ::Magpie::MultiMonitorUsage::Closest;
	::Magpie::CursorInterpolationMode cursorInterpolationMode = ::Magpie::CursorInterpolationMode::NearestNeighbor;

	// 10~1000
	float maxFrameRate = 60.0f;

	std::wstring launchParameters;

	uint32_t scalingFlags = ::Magpie::ScalingFlags::AdjustCursorSpeed | ::Magpie::ScalingFlags::DrawCursor;

	bool isPackaged = false;
	bool isCroppingEnabled = false;
	bool isAutoScale = false;
	bool isFrameRateLimiterEnabled = false;
};

}
