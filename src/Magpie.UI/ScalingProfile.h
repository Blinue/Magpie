#pragma once
#include "pch.h"
#include <Magpie.Core.h>


namespace winrt::Magpie::UI {

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
	::Magpie::Core::Cropping cropping{};
	::Magpie::Core::CaptureMode captureMode = ::Magpie::Core::CaptureMode::GraphicsCapture;
	uint32_t graphicsAdapter = 0;
	::Magpie::Core::MultiMonitorUsage multiMonitorUsage = ::Magpie::Core::MultiMonitorUsage::Nearest;
	::Magpie::Core::CursorInterpolationMode cursorInterpolationMode = ::Magpie::Core::CursorInterpolationMode::Nearest;

	uint32_t flags = ::Magpie::Core::MagFlags::IsVSync 
		| ::Magpie::Core::MagFlags::IsAdjustCursorSpeed
		| ::Magpie::Core::MagFlags::IsDrawCursor;
};

}
