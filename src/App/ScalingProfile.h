#pragma once
#include "pch.h"
#include <Runtime.h>


namespace winrt::Magpie::App {

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
struct ScalingProfile {
	void Copy(const ScalingProfile& other) noexcept {
		cursorScaling = other.cursorScaling;
		customCursorScaling = other.customCursorScaling;
		isCroppingEnabled = other.isCroppingEnabled;
		cropping = other.cropping;
		captureMode = other.captureMode;
		graphicsAdapter = other.graphicsAdapter;
		multiMonitorUsage = other.multiMonitorUsage;
		cursorInterpolationMode = other.cursorInterpolationMode;
		flags = other.flags;
	}

	DEFINE_MAGFLAG_ACCESSOR(IsDisableWindowResizing)
	DEFINE_MAGFLAG_ACCESSOR(Is3DGameMode)
	DEFINE_MAGFLAG_ACCESSOR(IsShowFPS)
	DEFINE_MAGFLAG_ACCESSOR(IsVSync)
	DEFINE_MAGFLAG_ACCESSOR(IsTripleBuffering)
	DEFINE_MAGFLAG_ACCESSOR(IsReserveTitleBar)
	DEFINE_MAGFLAG_ACCESSOR(IsAdjustCursorSpeed)
	DEFINE_MAGFLAG_ACCESSOR(IsDrawCursor)
	DEFINE_MAGFLAG_ACCESSOR(IsDisableDirectFlip)

	std::wstring name;

	// 若为打包应用，PathRule 存储 AUMID
	std::wstring pathRule;
	std::wstring classNameRule;
	bool isPackaged = false;
	
	CursorScaling cursorScaling = CursorScaling::NoScaling;
	float customCursorScaling = 1.0;

	bool isCroppingEnabled = false;
	::Magpie::Runtime::Cropping cropping{};
	::Magpie::Runtime::CaptureMode captureMode = ::Magpie::Runtime::CaptureMode::GraphicsCapture;
	uint32_t graphicsAdapter = 0;
	::Magpie::Runtime::MultiMonitorUsage multiMonitorUsage = ::Magpie::Runtime::MultiMonitorUsage::Nearest;
	::Magpie::Runtime::CursorInterpolationMode cursorInterpolationMode = ::Magpie::Runtime::CursorInterpolationMode::Nearest;

	uint32_t flags = ::Magpie::Runtime::MagFlags::IsVSync 
		| ::Magpie::Runtime::MagFlags::IsAdjustCursorSpeed
		| ::Magpie::Runtime::MagFlags::IsDrawCursor;
};

}
